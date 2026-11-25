# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Line-based JSON client for the probe's wire protocol."""

import itertools
import json
import socket
import threading
import time


class ProbeError(Exception):
    """An error response from the probe."""

    def __init__(self, code, message, is_method=False):
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message
        # Set on NoSuchProperty from ``get``: the name is a callable method.
        self.is_method = is_method


def _closed(method):
    return ConnectionError(f"probe closed the connection during '{method}'")


class ProbeClient:
    """Sends one request at a time over a connected stream socket.

    Each request waits at most ``timeout`` seconds (None: forever) for its
    answer and then raises TimeoutError; the client stays usable afterwards.
    A request that could not be sent completely leaves part of a line on the
    stream, so it closes the client: later requests raise ConnectionError
    naming the cause.
    """

    def __init__(self, sock, timeout=30.0):
        self._sock = sock
        self._buffer = bytearray()
        self._ids = itertools.count(1)
        self._lock = threading.Lock()
        self._closed = None  # why the client is closed, once it is
        self.timeout = timeout

    def _remaining(self, deadline, method):
        if deadline is None:
            return None
        if (remaining := deadline - time.monotonic()) <= 0:
            raise TimeoutError(
                f"probe did not answer '{method}' within {self.timeout} s"
            )
        return remaining

    def _readline(self, deadline, method):
        while (end := self._buffer.find(b"\n")) < 0:
            self._sock.settimeout(self._remaining(deadline, method))
            try:
                chunk = self._sock.recv(65536)
            except socket.timeout:
                continue  # _remaining raises on the next round
            except ConnectionError as err:
                raise _closed(method) from err
            if not chunk:
                raise _closed(method)
            self._buffer += chunk
        line = bytes(self._buffer[:end])
        del self._buffer[: end + 1]
        return line

    def _send(self, data, deadline, method):
        if self._closed is not None:
            raise ConnectionError(
                f"cannot send '{method}': the connection to the probe is closed "
                f"({self._closed})"
            )
        self._sock.settimeout(self._remaining(deadline, method))
        try:
            self._sock.sendall(data)
        except socket.timeout as err:
            cause = f"probe did not accept '{method}' within {self.timeout} s"
            self._close(cause)
            raise TimeoutError(cause) from err
        except ConnectionError as err:
            self._close(f"probe closed the connection during '{method}'")
            raise _closed(method) from err
        except BaseException as err:
            # interrupted, or failed, with part of the line possibly sent
            self._close(f"sending '{method}' failed: {type(err).__name__}: {err}")
            raise

    def _close(self, cause):
        self._closed = cause
        self._sock.close()

    def request(self, method, **params):
        with self._lock:
            deadline = None
            if self.timeout is not None:
                deadline = time.monotonic() + self.timeout
            request_id = next(self._ids)
            line = json.dumps(
                {"id": request_id, "method": method, "params": params},
                ensure_ascii=False,
            )
            self._send(line.encode("utf-8") + b"\n", deadline, method)
            while True:
                response = json.loads(self._readline(deadline, method).decode("utf-8"))
                if _answers(response, request_id):
                    break
                # Anything else is a late answer to an abandoned request.
        if "error" in response:
            error = response["error"]
            raise ProbeError(
                error.get("code", ""),
                error.get("message", ""),
                bool(error.get("isMethod", False)),
            )
        return response.get("result")

    def close(self):
        if self._closed is None:
            self._closed = "closed by the harness"
        self._sock.close()


def _answers(response, request_id):
    """Whether a response answers the request with the given id.

    A null id, explicitly given, with an error answers a request the probe
    could not parse.
    """
    if "id" not in response:
        return False
    if response["id"] is None:
        return "error" in response
    return response["id"] == request_id


_CURRENT = None


def set_current_client(client):
    global _CURRENT
    _CURRENT = client


def current_client():
    if _CURRENT is None:
        raise RuntimeError("no application is running (startApplication not called)")
    return _CURRENT
