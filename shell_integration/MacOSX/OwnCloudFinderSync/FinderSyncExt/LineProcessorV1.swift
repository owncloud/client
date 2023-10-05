//
//  LineProcessorV1.swift
//  FinderSyncExt
//
//  Created by Erik Verbruggen on 05-10-23.
//

import Foundation
import OSLog

class LineProcessorV1: NSObject, LineProcessor {
    let delegate: SyncClientDelegate

    @objc init(withDelegate delegate: SyncClientDelegate) {
        self.delegate = delegate
    }

    private func log(_ str: String, type logType: OSLogType) {
        // We cannot use OSLog's Logger class, because a lot of methods are only available in macOS 11.0 or higher.
        os_log("%@", type: logType, str)
    }

    /// Processes a line, where the trailing \n has already been stripped
    func process(line: String) {

        self.log("Processing line '\(line)'", type: .debug)
        let chunks = line.components(separatedBy: ":")
        let command = chunks[0]

        switch command {
        case "STATUS":
            let result = chunks[1]
            let path = chunks.suffix(from: 2).joined(separator: ":")
            DispatchQueue.main.async { self.delegate.setResultForPath(path, result: result) }
        case "UPDATE_VIEW":
            let path = chunks[1]
            DispatchQueue.main.async { self.delegate.reFetchFileNameCache(forPath: path) }
        case "REGISTER_PATH":
            let path = chunks[1]
            DispatchQueue.main.async { self.delegate.registerPath(path) }
        case "UNREGISTER_PATH":
            let path = chunks[1]
            DispatchQueue.main.async { self.delegate.unregisterPath(path) }
        case "GET_STRINGS":
            // BEGIN and END messages, do nothing.
            break
        case "STRING":
            DispatchQueue.main.async { self.delegate.setString(chunks[1], value: chunks[2]) }
        case "GET_MENU_ITEMS":
            if chunks[1] == "BEGIN" {
                DispatchQueue.main.async { self.delegate.resetMenuItems() }
            } else {
                // Do NOT run this on the main queue! It might be blocked waiting for the menu to be completed.
                delegate.menuHasCompleted()
            }
        case "MENU_ITEM":
            let item = [
                "command" : chunks[1],
                "flags"   : chunks[2],
                "text"    : chunks[3]
            ]
            DispatchQueue.main.async { self.delegate.addMenuItem(item) }
        default:
            self.log("Unknown command '\(command)'", type: .error)
        }
    }
}
