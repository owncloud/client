# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Stand-in application under test.

It serves the fake probe on the Unix socket named by ``QT_GUI_TEST_PROBE``,
the way the real probe's QLocalServer does, and keeps running after the
harness disconnects, so ``detach()`` has to terminate it.

``FAKE_AUT_ARGV`` names a file that receives the arguments and working
directory as JSON. ``FAKE_AUT_DELAY`` delays listening by that many seconds.
"""

import json
import os
import socket
import sys
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE.parent), str(HERE)]

from fake_probe import FakeProbe  # pylint: disable=wrong-import-position


def main():
    if argv_file := os.environ.get("FAKE_AUT_ARGV"):
        with open(argv_file, "w", encoding="utf-8") as out:
            json.dump({"argv": sys.argv[1:], "cwd": os.getcwd()}, out)
    time.sleep(float(os.environ.get("FAKE_AUT_DELAY", "0")))
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(os.environ["QT_GUI_TEST_PROBE"])
    server.listen(1)
    conn, _ = server.accept()
    FakeProbe(sock=conn, pid=os.getpid())
    threading.Event().wait()


if __name__ == "__main__":
    main()
