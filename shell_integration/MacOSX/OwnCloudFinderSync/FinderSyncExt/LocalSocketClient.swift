//
//  LocalSocketClient.swift
//  FinderSyncExt
//
//  Created by Erik Verbruggen <erik@verbruggen.consulting> on 05-10-23.
//

import Foundation
import OSLog

/// Process lines from the `LocalSocketClient`.
@objc protocol LineProcessor {
    func process(line: String);
}

/// Class handling the (asynchronous) communication with a server over a local (UNIX) socket.
///
/// The implementation uses a `DispatchQueue` and `DispatchSource`s to handle asynchronous communication and thread
/// safety. All public/@objc function can be called from any thread/queue. The delegate that handles the
/// line-decoding is **not invoked on the UI thread**, but the (random) thread associated with the `DispatchQueue`!
/// If any UI work needs to be done, the class implementing the `LineProcessor` protocol should dispatch this work
/// on the main queue (so the UI thread) itself!
///
/// Other than the `init(withSocketPath:, lineProcessor)` and the `start()` method, all work is done "on the dispatch
/// queue". The `localSocketQueue` is a serial dispatch queue (so a maximum of 1, and only 1, task is run at any
/// moment), which guarantees safe access to instance variables. Both `askOnSocket(_:, query:)` and
/// `askForIcon(_:, isDirectory:)` will internally dispatch the work on the `DispatchQueue`.
///
/// Sending and receiving data to and from the socket, is handled by two `DispatchSource`s. These will run an event
/// handler when data can be read from resp. written to the socket. These handlers will also be run on the
/// `DispatchQueue`.
class LocalSocketClient: NSObject {
    let socketPath: String
    let lineProcessor: LineProcessor

    private var sock: Int32?
    private var localSocketQueue = DispatchQueue.init(label: "localSocketQueue")
    private var readSource: DispatchSourceRead?
    private var writeSource: DispatchSourceWrite?
    private var inBuffer = [UInt8]()
    private var outBuffer = [UInt8]()

    @objc var isConnected: Bool {
        get {
            sock != nil
        }
    }

    @objc init(withSocketPath socketPath: String, lineProcessor: LineProcessor) {
        self.socketPath = socketPath
        self.lineProcessor = lineProcessor

        super.init()

        self.inBuffer.reserveCapacity(1000)
    }

    private func log(_ str: String, type logType: OSLogType) {
        if #available(macOSApplicationExtension 11.0, *) {
            // NOTE: when support for 10.* is dropped, make an instance variable instead of instantiating the `Logger`
            //       object every time.
            Logger().log(level: logType, "\(str, privacy: .public)")
        } else {
            os_log("%@", type: logType, str)
        }
    }

    @objc func start() {
        var sa_un = sockaddr_un()

        let socketPathByteCount = socketPath.utf8.count + 1; // add 1 for the NUL terminator char
        let maxByteCount = MemoryLayout.size(ofValue: sa_un.sun_path)
        guard socketPathByteCount < maxByteCount else {
            log("Socket path '\(socketPath)' is too long: \(socketPathByteCount) is longer than \(maxByteCount)",
                type: .error)
            return
        }

        log("Opening local socket...", type: .debug)

        self.sock = socket(AF_LOCAL, SOCK_STREAM, 0)
        guard self.sock != -1 else {
            self.log("Cannot open socket: \(self.strErr())", type: .error)
            self.restart()
            return
        }

        log("Local socket openned, now connecting to '\(self.socketPath)' ...", type: .debug)

        sa_un.sun_family = UInt8(AF_LOCAL & 0xff)

        let pathBytes = socketPath.utf8 + [0]
        pathBytes.withUnsafeBytes { srcBuffer in
            withUnsafeMutableBytes(of: &sa_un.sun_path) { dstPtr in
                dstPtr.copyMemory(from: srcBuffer)
            }
        }

        let connStatus = withUnsafePointer(to: &sa_un) { sa_unPtr in
            // We are now allowed to mess with the raw pointer to `sa_un`, and cast it to a `sockaddr` pointer.
            // This is basically a barrier before and after this closure, so that all writes have been done before by
            // the compiler, and that subsequent reads do not read old values (because Swift can't see if `connect`
            // messes with the memory for which it receives a raw pointer).
            sa_unPtr.withMemoryRebound(to: sockaddr.self, capacity: 1) { saPtr in
                connect(self.sock!, saPtr, socklen_t(MemoryLayout<sockaddr_un>.size))
            }
        }

        guard connStatus != -1 else {
            self.log("Cannot connect to '\(self.socketPath): \(self.strErr())", type: .error)
            self.restart()
            return
        }

        let flags = fcntl(self.sock!, F_GETFL, 0)
        guard -1 != fcntl(self.sock!, F_SETFL, flags | O_NONBLOCK) else {
            self.log("Cannot set socket to non-blocking mode: \(self.strErr())", type: .error)
            self.restart()
            return
        }

        self.readSource = DispatchSource.makeReadSource(fileDescriptor: self.sock!, queue: self.localSocketQueue)
        self.readSource!.setEventHandler { self.readFromSocket() }
        self.readSource!.setCancelHandler {
            self.readSource = nil
            self.closeConnection()
        }
        self.readSource!.resume()

        self.writeSource = DispatchSource.makeWriteSource(fileDescriptor: self.sock!, queue: self.localSocketQueue)
        self.writeSource!.setEventHandler { self.writeToSocket() }
        self.writeSource!.setCancelHandler {
            self.writeSource = nil
            self.closeConnection()
        }
        // The writeSource dispatch queue starts suspended; we will resume it when we have data to send (and suspend it
        // again when our send buffer is empty).
        self.log("We have a connection! Starting I/O channel...", type: .info)

        self.askOnSocket("", query: "GET_STRINGS")
    }

    private func restart() {
        self.closeConnection()

        DispatchQueue.main.async {
            Timer.scheduledTimer(withTimeInterval: 5, repeats: false, block: { _ in
                self.start()
            });
        }
    }

    private func closeConnection() {
        self.readSource?.cancel()
        self.writeSource?.cancel()
        self.readSource = nil
        self.writeSource = nil
        self.inBuffer.removeAll()
        self.outBuffer.removeAll()
        if let sock = self.sock {
            close(sock)
            self.sock = nil
        }
    }

    private func strErr() -> String {
        let err = errno // copy error code now, in case something else happens
        return String(utf8String: strerror(err)) ?? "Unknown error code  (\(err))"
    }

    @objc func askOnSocket(_ path: String, query verb: String) {
        let line = "\(verb):\(path)\n"
        self.localSocketQueue.async {
            guard self.isConnected else {
                // socket was closed while work was still scheduled on the queue
                return
            }

            self.log("Sending line '\(line)", type: .debug)

            let writeSourceIsSuspended = self.outBuffer.isEmpty
            let uint8Data: [UInt8] = line.utf8 + []
            self.outBuffer.append(contentsOf: uint8Data)

            // Weird stuff happens when you call resume when the DispatchSource is already resumed, so: if we did NOT
            // have any data in our output buffer before queueing more data, it must be suspended.
            if writeSourceIsSuspended {
                self.writeSource?.resume() // now we will get notified when we can write to the socket.
            }
        }
    }

    private func writeToSocket() {
        guard self.isConnected else {
            // socket was closed while work was still scheduled on the queue
            return
        }

        guard !self.outBuffer.isEmpty else {
            // the buffer is empty, suspend you-can-write-data notifications
            self.writeSource!.suspend()
            return
        }

        let totalAmountOfBytes = self.outBuffer.count
        let bytesWritten = self.outBuffer.withUnsafeBytes { ptr in
            write(self.sock!, ptr.baseAddress, totalAmountOfBytes)
        }
        if bytesWritten == -1 {
            if errno == EAGAIN {
                // no space free in the buffer on the OS side, we're done
            } else {
                self.log("Error writing to local socket: \(self.strErr())", type: .error)
                self.restart()
            }
        } else if bytesWritten > 0 {
            self.outBuffer.removeFirst(bytesWritten)
            if self.outBuffer.isEmpty {
                // the buffer is empty, suspend you-can-write-data notifications
                self.writeSource!.suspend()
            }
        }
    }

    @objc func askForIcon(_ path: String, isDirectory: Bool) {
        let verb = isDirectory ? "RETRIEVE_FOLDER_STATUS" : "RETRIEVE_FILE_STATUS"
        self.askOnSocket(path, query: verb)
    }

    private func readFromSocket() {
        guard self.isConnected else {
            // socket was closed while work was still scheduled on the queue
            return
        }

        let bufferLength = self.inBuffer.capacity / 2
        var buffer = [UInt8].init(repeating: 0, count: bufferLength)

        while true {
            let bytesRead = buffer.withUnsafeMutableBytes { ptr in
                read(self.sock!, ptr.baseAddress, bufferLength)
            }
            if bytesRead == -1 {
                if errno == EAGAIN {
                    return // no bytes available, and no error, so we're done
                } else {
                    self.log("Error reading from local socket: \(self.strErr())", type: .error)
                    self.closeConnection()
                    return // we've closed the connection, we're done
                }
            } else {
                self.inBuffer.append(contentsOf: buffer[0..<bytesRead])
                self.processInBuffer()
                // see if there is more to read: restart the loop
            }
        }
    }

    private func processInBuffer() {
        let separator: UInt8 = 0xa // byte value for '\n'
        while true {
            if let pos = self.inBuffer.firstIndex(of: separator) {
                self.inBuffer[pos] = 0 // add NUL terminator
                let newLine = String(cString: &self.inBuffer)
                self.inBuffer.removeFirst(pos + 1) // remove the line from the buffer, including the NUL terminator
                self.lineProcessor.process(line: newLine)
            } else {
                // no separator, we're done
                return;
            }
        }
    }
}
