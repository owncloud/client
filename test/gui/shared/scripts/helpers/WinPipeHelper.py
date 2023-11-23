import os
import time
import win32pipe, win32file, winerror, pywintypes, win32event


def get_pipe_path():
    pipename = r"\\.\\pipe\\"
    pipename = os.path.join(pipename, "ownCloud-" + os.getenv("USERNAME"))
    return pipename


class WinPipeConnect():
    def __init__(self):
        self.connected = False
        self._pipe = None
        self._remainder = ''.encode('utf-8')
        self.connectToPipeServer()


    def connectToPipeServer(self):
        try:
            pipename = get_pipe_path()

            self._pipe = win32file.CreateFile(
                pipename,
                win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                0,
                None,
                win32file.OPEN_EXISTING,
                win32file.FILE_FLAG_OVERLAPPED,
                None
            )

            if self._pipe == win32file.INVALID_HANDLE_VALUE:
                win32pipe.CloseHandle(self._pipe)
                raise Exception("Invalid _pipe value")

            self.connected = True

            self.sendCommand('VERSION:\n')
            self.sendCommand('GET_STRINGS:\n')
        except Exception as e:
            print("Could not connect to named pipe " + pipename + "\n" + str(e))
            win32file.CloseHandle(self._pipe)


    def sendCommand(self, cmd):
        if self.connected:
            try:
                win32file.WriteFile(self._pipe, cmd.encode('utf-8'))
            except Exception as e:
                print(str(e))
        else:
            print("Cannot send, not connected!")


    # Reads data that becomes available.
    # New responses can be accessed with get_available_responses().
    def read_socket_data_with_timeout(self, timeout):
        messages = b''
        start_time = time.time()
        while True:
            if (time.time() - start_time) >= timeout:
                self._remainder += messages
                break

            overlapped = pywintypes.OVERLAPPED()
            buffer = win32file.AllocateReadBuffer(1024)
            # Enable asynchronous reading
            try:
                result, data = win32file.ReadFile(self._pipe, buffer, overlapped)

                if result == winerror.ERROR_IO_PENDING:
                    # Wait for read completion or timeout
                    res = win32event.WaitForSingleObject(self._pipe, int(timeout * 1000))
                    if res == win32event.WAIT_OBJECT_0:
                        num_bytes = win32file.GetOverlappedResult(self._pipe, overlapped, True)
                        messages += bytes(buffer[:num_bytes])
                else:
                    # store decodable messages
                    bytes(data).decode('utf-8')
                    messages += bytes(data)
            except UnicodeDecodeError as e:
                pass
            except Exception as e:
                print(str(e))


    # Parses response lines out of collected data, returns list of strings
    def get_available_responses(self):
        end = self._remainder.rfind(b'\n')
        if end == -1:
            return []
        data = self._remainder[:end]
        self._remainder = self._remainder[end+1:]
        return data.decode('utf-8').split('\n')


    def close_conn(self):
        win32file.CloseHandle(self._pipe)