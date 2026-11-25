# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Stand-in application under test.

It serves the fake probe on the Unix socket named by ``QT_GUI_TEST_PROBE``,
the way the real probe's QLocalServer does, and keeps running after the
harness disconnects, so ``detach()`` has to terminate it.

``FAKE_AUT_ARGV`` names a file that receives the arguments and working
directory as JSON. ``FAKE_AUT_DELAY`` delays listening by that many seconds.
``FAKE_AUT_SAY`` is written as a line "<text> on stdout" to stdout and
"<text> on stderr" to stderr. ``FAKE_AUT_LAST_WORDS`` is written to stderr
by a child process a moment after SIGTERM, which ends the application.
"""

import json
import os
import signal
import socket
import sys
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent), str(HERE)]

from fake_probe import FakeProbe  # pylint: disable=wrong-import-position


def _last_words(text):
    def say(*_):
        # from a child, after the application itself exited
        if not os.fork():
            time.sleep(0.2)
            print(text, file=sys.stderr, flush=True)
        os._exit(0)

    signal.signal(signal.SIGTERM, say)


def main():
    if last_words := os.environ.get("FAKE_AUT_LAST_WORDS"):
        _last_words(last_words)
    if argv_file := os.environ.get("FAKE_AUT_ARGV"):
        with open(argv_file, "w", encoding="utf-8") as out:
            json.dump({"argv": sys.argv[1:], "cwd": os.getcwd()}, out)
    if say := os.environ.get("FAKE_AUT_SAY"):
        print(f"{say} on stdout", flush=True)
        print(f"{say} on stderr", file=sys.stderr, flush=True)
    time.sleep(float(os.environ.get("FAKE_AUT_DELAY", "0")))
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(os.environ["QT_GUI_TEST_PROBE"])
    server.listen(1)
    conn, _ = server.accept()
    FakeProbe(sock=conn, pid=os.getpid())
    threading.Event().wait()


if __name__ == "__main__":
    main()
