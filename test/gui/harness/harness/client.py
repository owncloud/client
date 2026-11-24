# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Line-based JSON client for the probe's wire protocol."""

import itertools
import json
import threading


class ProbeError(Exception):
    """An error response from the probe."""

    def __init__(self, code, message):
        super().__init__(f"{code}: {message}")
        self.code = code
        self.message = message


class ProbeClient:
    """Sends one request at a time over a connected stream socket."""

    def __init__(self, sock):
        self._sock = sock
        self._reader = sock.makefile("rb")
        self._ids = itertools.count(1)
        self._lock = threading.Lock()

    def request(self, method, **params):
        with self._lock:
            request_id = next(self._ids)
            line = json.dumps(
                {"id": request_id, "method": method, "params": params},
                ensure_ascii=False,
            )
            self._sock.sendall(line.encode("utf-8") + b"\n")
            while True:
                if not (raw := self._reader.readline()):
                    raise ConnectionError(
                        f"probe closed the connection during '{method}'"
                    )
                response = json.loads(raw.decode("utf-8"))
                # Anything else is a late answer to an abandoned request.
                if response.get("id") == request_id:
                    break
        if "error" in response:
            error = response["error"]
            raise ProbeError(error.get("code", ""), error.get("message", ""))
        return response.get("result")

    def close(self):
        self._reader.close()
        self._sock.close()


_CURRENT = None


def set_current_client(client):
    global _CURRENT
    _CURRENT = client


def current_client():
    if _CURRENT is None:
        raise RuntimeError("no application is running (startApplication not called)")
    return _CURRENT
