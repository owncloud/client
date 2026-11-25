# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Starting the application under test and its application contexts."""

# Squish's names are dictated by the suite.
# pylint: disable=invalid-name

import atexit
import collections
import importlib
import itertools
import os
import re
import shlex
import socket
import subprocess
import sys
import tempfile
import threading
import time

from harness import client as client_module

# Squish's AUTPMTimeout: how long an application may take to open its probe.
DEFAULT_START_TIMEOUT_MS = 15000
_TERMINATE_TIMEOUT = 10
_STDERR_TAIL_LINES = 20
_RETRY_INTERVAL = 0.05
# how long detach() waits for the rest of stderr; a child process that
# inherited it may hold it open for much longer
_DRAIN_TIMEOUT = 0.5
_LOG_NAME = re.compile(r"[0-9]{2,}-.*\.log")  # as _open_log names them

_AUT_PATHS = {}
_ENV = {}
_START_TIMEOUT_MS = DEFAULT_START_TIMEOUT_MS
_REQUEST_TIMEOUT = 30.0
_CONTEXTS = []
_CURRENT = None
_SOCKET_NUMBERS = itertools.count(1)
_LOG_DIR = None
_LOG_NUMBERS = collections.Counter()  # launches so far, per log directory
_NEW_LOGS = []  # log files of the launches take_new_logs() has not returned


def configure(
    aut_paths,
    env,
    start_timeout_ms=DEFAULT_START_TIMEOUT_MS,
    request_timeout=30.0,
    log_dir=None,
):
    """Set the applications (name -> directory, as in server.ini) and their env.

    ``env`` is added to the harness's own environment, which already holds
    the suite's ENVVARS. ``request_timeout`` is the probe request timeout in
    seconds. With ``log_dir``, each launch writes its stdout and stderr to
    ``log_dir/<test case>/<NN>-<name>.log`` instead of the harness's own;
    NN counts the launches in that test case.
    """
    global _AUT_PATHS, _ENV, _START_TIMEOUT_MS, _REQUEST_TIMEOUT, _LOG_DIR
    _AUT_PATHS = dict(aut_paths)
    _ENV = dict(env)
    _START_TIMEOUT_MS = start_timeout_ms
    _REQUEST_TIMEOUT = request_timeout
    _LOG_DIR = log_dir
    _LOG_NUMBERS.clear()
    _NEW_LOGS.clear()


class _StderrTail:
    """Copies the application's stderr through and keeps its last lines.

    The lines go to ``sink``, a binary file closed at the end of the stream
    or by ``close_sink()``, or else to the harness's stderr. Lines after
    ``close_sink()`` go to the harness's stderr too, marked with the log's
    name.
    """

    def __init__(self, stream, sink=None):
        self.lines = collections.deque(maxlen=_STDERR_TAIL_LINES)
        self._stream = stream
        self._logged = sink is not None
        self._sink = sink
        self._label = os.path.basename(sink.name) if sink else ""
        self._lock = threading.Lock()  # the sink, between reader and close_sink
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        try:
            with self._stream:
                for raw in self._stream:
                    line = raw.decode("utf-8", "replace")
                    self.lines.append(line.rstrip("\n"))
                    self._write(raw, line)
        finally:
            self.close_sink()

    def _write(self, raw, line):
        if not self._logged:
            sys.stderr.write(line)
            return
        with self._lock:
            if not self._sink:
                sys.stderr.write(f"[{self._label} after detach] {line}")
                sys.stderr.flush()
                return
            try:
                self._sink.write(raw)
                return
            except OSError as exc:  # as a full disk: keep reading, or it blocks
                self._sink.close()
                self._sink = None
                self._logged = False
                sys.stderr.write(f"[WARNING] cannot write the log: {exc}\n")
        sys.stderr.write(line)

    def close_sink(self):
        """Close the sink; the reader goes on with the harness's stderr."""
        with self._lock:
            if self._sink:
                self._sink.close()
                self._sink = None

    def wait(self, timeout):
        """Wait for the end of the stream, at most ``timeout`` seconds."""
        self._thread.join(timeout)

    def text(self, wait=2.0):
        """The tail; waits a little for the reader once the process exited."""
        self._thread.join(wait)
        return "\n".join(self.lines)


class _StdoutCopy:  # pylint: disable=too-few-public-methods
    """Copies the application's stdout to the harness's ``sys.stdout``, a
    line at a time, looking it up for each line: so that the console
    reporter's capture gets it, as it gets the harness's own output."""

    def __init__(self, stream):
        self._stream = stream
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def _run(self):
        with self._stream:
            for raw in self._stream:
                try:
                    sys.stdout.write(raw.decode("utf-8", "replace"))
                    sys.stdout.flush()
                except (OSError, ValueError):  # closed: keep reading, or it blocks
                    pass

    def wait(self, timeout):
        """Wait for the end of the stream, at most ``timeout`` seconds."""
        self._thread.join(timeout)


class ApplicationContext:  # pylint: disable=too-many-instance-attributes
    """One started application and the connection to its probe."""

    def __init__(  # pylint: disable=too-many-arguments
        self,
        name,
        process,
        client,
        pid,
        socket_path,
        *,
        stderr=None,
        stdout=None,
        log_path=None,
    ):
        self.name = name
        self.pid = pid
        self.client = client
        # where its output goes, or None when it goes to the harness's own
        self.log_path = log_path
        self._process = process
        self._socket_path = socket_path
        # the _StderrTail, and the _StdoutCopy without a log
        self._stderr, self._stdout_copy = stderr, stdout

    @property
    def isRunning(self):
        return self._process.poll() is None

    def detach(self):
        """Disconnect, and terminate the application if it still runs."""
        try:
            self.client.close()
        except OSError:
            pass
        _stop(self._process)
        # the rest of the output through, but not for as long as a child
        # process may hold it open; then the log is closed
        deadline = time.monotonic() + _DRAIN_TIMEOUT
        for stream in (self._stderr, self._stdout_copy):
            if stream:
                stream.wait(max(0.0, deadline - time.monotonic()))
        if self._stderr:
            self._stderr.close_sink()
        _remove(self._socket_path)
        _forget(self)

    def __repr__(self):
        state = "running" if self.isRunning else "exited"
        return f"<ApplicationContext {self.name} pid={self.pid} {state}>"


def _stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(_TERMINATE_TIMEOUT)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def _remove(path):
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass


def _forget(ctx):
    global _CURRENT
    if ctx in _CONTEXTS:
        _CONTEXTS.remove(ctx)
    if _CURRENT is ctx:
        _CURRENT = _CONTEXTS[-1] if _CONTEXTS else None
        client_module.set_current_client(_CURRENT.client if _CURRENT else None)


def _executable(name):
    if name not in _AUT_PATHS:
        known = ", ".join(sorted(_AUT_PATHS)) or "none"
        raise RuntimeError(f"unknown application '{name}' (configured: {known})")
    directory = _AUT_PATHS[name]
    return directory, os.path.join(directory, name)


def _open_log(name):
    """The log file of a new launch of ``name``, emptied and opened to append."""
    test_case = os.path.basename(importlib.import_module("squishinfo").testCase)
    directory = os.path.join(_LOG_DIR, test_case or "no-test-case")
    os.makedirs(directory, exist_ok=True)
    if not _LOG_NUMBERS[directory]:
        _remove_old_logs(directory)
    _LOG_NUMBERS[directory] += 1
    path = os.path.join(directory, f"{_LOG_NUMBERS[directory]:02d}-{name}.log")
    # Emptied, as a log of an earlier run with the same number; appending and
    # unbuffered, as the application writes to it through its stdout too.
    return open(
        path,
        "ab",
        buffering=0,
        opener=lambda p, flags: os.open(p, flags | os.O_TRUNC, 0o666),
    )


def _remove_old_logs(directory):
    """Remove the logs an earlier run left in a test case's log directory."""
    for entry in os.scandir(directory):
        if _LOG_NAME.fullmatch(entry.name) and entry.is_file():
            _remove_log(entry.path)


def _remove_log(path):
    """Remove a log; one that cannot be removed is only warned about."""
    try:
        _remove(path)
    except OSError as exc:
        print(f"[WARNING] cannot remove the log: {exc}", file=sys.stderr, flush=True)


def _discard_log(log):
    """Remove the log of a launch that did not start, and free its number.

    It does not raise, so the launch's own error is the one reported.
    """
    _LOG_NUMBERS[os.path.dirname(log.name)] -= 1
    try:
        log.close()
    except OSError:
        pass
    _remove_log(log.name)


def take_new_logs():
    """The log files of the launches since the last call, oldest first."""
    logs = list(_NEW_LOGS)
    _NEW_LOGS.clear()
    return logs


def _connect(name, process, socket_path, stderr):
    """Connect to the probe socket, retrying until it is open."""
    deadline = time.monotonic() + _START_TIMEOUT_MS / 1000
    while True:
        if (code := process.poll()) is not None:
            raise RuntimeError(
                f"'{name}' exited with exit code {code} before its probe "
                f"listened on {socket_path}; stderr:\n{stderr.text()}"
            )
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            sock.connect(socket_path)
            return sock
        except (FileNotFoundError, ConnectionRefusedError):
            sock.close()
        if time.monotonic() >= deadline:
            _stop(process)
            raise RuntimeError(
                f"'{name}' did not open its probe socket {socket_path} within "
                f"{_START_TIMEOUT_MS} ms (terminated, exit code "
                f"{process.returncode}); is it built with the probe? "
                f"stderr:\n{stderr.text()}"
            )
        time.sleep(_RETRY_INTERVAL)


def startApplication(cmdline):
    """Start an application by its configured name and connect to its probe."""
    global _CURRENT
    if not (args := shlex.split(cmdline)):
        raise RuntimeError("startApplication needs a command line")
    name = args[0]
    directory, executable = _executable(name)
    socket_path = os.path.join(
        tempfile.gettempdir(), f"qtguiprobe-{os.getpid()}-{next(_SOCKET_NUMBERS)}"
    )
    _remove(socket_path)
    env = {**os.environ, **_ENV, "QT_GUI_TEST_PROBE": socket_path}
    log = _open_log(name) if _LOG_DIR else None
    try:
        # pylint: disable-next=consider-using-with
        process = subprocess.Popen(
            [executable, *args[1:]],
            cwd=directory,
            env=env,
            # without a log, through the harness's sys.stdout, not its fd 1
            stdout=log or subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except BaseException:
        if log:
            _discard_log(log)
        raise
    if log:
        _NEW_LOGS.append(log.name)
    # from here on, it closes the log
    stderr = _StderrTail(process.stderr, sink=log)
    stdout = None if log else _StdoutCopy(process.stdout)
    try:
        sock = _connect(name, process, socket_path, stderr)
        client = client_module.ProbeClient(sock, timeout=_REQUEST_TIMEOUT)
        try:
            pid = client.request("hello")["pid"]
        except BaseException:
            client.close()
            raise
    except BaseException:
        _stop(process)
        _remove(socket_path)
        raise
    ctx = ApplicationContext(
        name,
        process,
        client,
        pid,
        socket_path,
        stderr=stderr,
        stdout=stdout,
        log_path=log.name if log else None,
    )
    _CONTEXTS.append(ctx)
    _CURRENT = ctx
    client_module.set_current_client(client)
    return ctx


def applicationContextList():
    return list(_CONTEXTS)


def currentApplicationContext():
    return _CURRENT


def release_all():
    """Let every application drop the handles it gave out.

    Objects are found anew afterwards. An application that closed its
    connection or stopped answering has nothing to release. Neither has one
    whose connection failed otherwise or that refused, which is only
    logged: the scenario does not fail for it.
    """
    for ctx in list(_CONTEXTS):
        try:
            ctx.client.request("release")
        except (ConnectionError, TimeoutError):
            pass
        except (OSError, client_module.ProbeError) as exc:
            print(
                f"[WARNING] '{ctx.name}' did not release its handles: "
                f"{type(exc).__name__}: {exc}",
                file=sys.stderr,
                flush=True,
            )


def detach_all():
    """Detach every application; also run at exit so none is left behind."""
    for ctx in list(_CONTEXTS):
        ctx.detach()


atexit.register(detach_all)
