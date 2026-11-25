# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
# pylint: disable=redefined-outer-name

import json
import socket
import threading
import time

import pytest

from harness.client import ProbeClient, ProbeError


@pytest.fixture
def pair():
    ours, theirs = socket.socketpair()
    yield ours, theirs
    ours.close()
    theirs.close()


def _answer(sock, *responses):
    """Read one request, then send the given responses (``id`` None: echo)."""

    def run():
        request = json.loads(sock.makefile("rb").readline())
        for response in responses:
            if "id" not in response:
                response = {"id": request["id"], **response}
            sock.sendall(json.dumps(response).encode() + b"\n")

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return thread


def test_request_times_out_naming_the_method(pair):
    ours, _theirs = pair
    client = ProbeClient(ours, timeout=0.2)
    start = time.monotonic()
    with pytest.raises(TimeoutError, match="'hello'"):
        client.request("hello")
    assert time.monotonic() - start < 2


def test_default_timeout_is_thirty_seconds(pair):
    assert ProbeClient(pair[0]).timeout == 30.0


def test_closed_connection_raises_connection_error(pair):
    ours, theirs = pair
    theirs.close()
    with pytest.raises(ConnectionError, match="'find'"):
        ProbeClient(ours, timeout=2).request("find", selector={})


def test_late_answer_to_an_abandoned_request_is_skipped(pair):
    ours, theirs = pair
    _answer(theirs, {"id": 99, "result": "stale"}, {"result": "fresh"})
    assert ProbeClient(ours, timeout=2).request("hello") == "fresh"


def test_client_stays_usable_after_a_timeout(pair):
    ours, theirs = pair
    client = ProbeClient(ours, timeout=0.2)
    with pytest.raises(TimeoutError):
        client.request("hello")
    reader = theirs.makefile("rb")
    first = json.loads(reader.readline())
    thread = _answer(theirs, {"result": "second"})
    # The late answer to the first request arrives before the second one.
    theirs.sendall(json.dumps({"id": first["id"], "result": "x"}).encode() + b"\n")
    assert client.request("hello") == "second"
    thread.join(2)


def test_error_without_id_is_raised_not_waited_for(pair):
    ours, theirs = pair
    _answer(theirs, {"id": None, "error": {"code": "BadRequest", "message": "bad"}})
    with pytest.raises(ProbeError, match="BadRequest"):
        ProbeClient(ours, timeout=2).request("hello")


def _answer_raw(sock, *lines):
    """Read one request, then send the given lines; ``{id}`` is its id."""

    def run():
        request = json.loads(sock.makefile("rb").readline())
        for line in lines:
            sock.sendall(line.replace("{id}", str(request["id"])).encode() + b"\n")

    thread = threading.Thread(target=run, daemon=True)
    thread.start()
    return thread


def test_answer_without_id_is_not_taken_for_the_request(pair):
    ours, theirs = pair
    _answer_raw(
        theirs,
        '{"result": "no id"}',
        '{"id": null, "result": "null id without an error"}',
        '{"id": {id}, "result": "fresh"}',
    )
    assert ProbeClient(ours, timeout=2).request("hello") == "fresh"


def test_send_timeout_closes_the_client(pair):
    ours, theirs = pair
    ours.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4096)
    client = ProbeClient(ours, timeout=0.2)
    # Nobody reads: the request cannot be sent completely.
    with pytest.raises(TimeoutError, match="did not accept 'find'"):
        client.request("find", selector={"text": "x" * (16 << 20)})
    with pytest.raises(ConnectionError, match="did not accept 'find'"):
        client.request("hello")
    # Nothing more was written after the fragment of the first request.
    theirs.setblocking(False)
    received = b""
    while True:
        try:
            if not (chunk := theirs.recv(1 << 20)):
                break
        except BlockingIOError:
            break
        received += chunk
    assert b'"hello"' not in received


def test_closed_client_raises_connection_error(pair):
    client = ProbeClient(pair[0], timeout=2)
    client.close()
    with pytest.raises(ConnectionError, match="closed"):
        client.request("hello")
