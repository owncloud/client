# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's ``test`` module: verification points and logging."""

# Squish's names are dictated by the suite: camelCase functions; the module
# itself shadows the standard library's ``test`` package.
# pylint: disable=invalid-name

from harness import results
from harness.results import Results
from harness.settings import test_settings

__all__ = [
    "Results",
    "compare",
    "fail",
    "log",
    "resultCount",
    "startVideoCapture",
    "stopVideoCapture",
    "verify",
    "warning",
]


def _print(kind, message, detail=""):
    text = f"[{kind.upper()}] {message}"
    if detail:
        text += f" ({detail})"
    print(text, flush=True)


def _check(ok, message, detail):
    """Record a pass or a fail; raise on a fail if throwOnFailure is set."""
    kind = "passes" if ok else "fails"
    results.record(kind, message, detail)
    if not ok:
        _print("fail", message, detail)
        if test_settings.throwOnFailure:
            raise AssertionError(f"{message}: {detail}" if detail else message)
    return ok


def compare(value1, value2, message=""):
    """Pass if both values are equal."""
    ok = value1 == value2
    detail = f"{value1!r} == {value2!r}" if ok else f"{value1!r} != {value2!r}"
    return _check(ok, message or "Comparison", detail)


def verify(condition, message=""):
    """Pass if the condition is true."""
    return _check(bool(condition), message or "Verified", repr(condition))


def fail(message, detail=""):
    """Record a failure."""
    _check(False, message, detail)


def warning(message):
    results.record("warnings", message)
    _print("warning", message)


def log(message):
    results.record("log", message)
    _print("log", message)


def resultCount(kind):
    """Results of a kind ("passes", "fails", "errors", "warnings") in this test case."""
    return results.result_count(kind)


def startVideoCapture():
    log("video capture is not supported; not recording")


def stopVideoCapture():
    log("video capture is not supported; nothing to stop")
