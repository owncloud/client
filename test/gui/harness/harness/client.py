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
    """

    def __init__(self, sock, timeout=30.0):
        self._sock = sock
        self._buffer = bytearray()
        self._ids = itertools.count(1)
        self._lock = threading.Lock()
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
            self._sock.settimeout(self._remaining(deadline, method))
            try:
                self._sock.sendall(line.encode("utf-8") + b"\n")
            except socket.timeout as err:
                raise TimeoutError(
                    f"probe did not accept '{method}' within {self.timeout} s"
                ) from err
            except ConnectionError as err:
                raise _closed(method) from err
            while True:
                response = json.loads(self._readline(deadline, method).decode("utf-8"))
                # A null id answers a request the probe could not parse.
                if response.get("id") in (request_id, None):
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
        self._sock.close()


_CURRENT = None


def set_current_client(client):
    global _CURRENT
    _CURRENT = client


def current_client():
    if _CURRENT is None:
        raise RuntimeError("no application is running (startApplication not called)")
    return _CURRENT
