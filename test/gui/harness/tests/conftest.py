# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Fixtures shared by the suite-running tests."""

import builtins
import os
import sys

import pytest
import squishinfo

from harness import aut, results


@pytest.fixture
def isolated(tmp_path):
    """Undo what a suite run changes in the process: env, cwd, paths, builtins,
    and the applications it configured."""
    environ = dict(os.environ)
    cwd = os.getcwd()
    path = list(sys.path)
    names = dict(vars(builtins))
    modules = set(sys.modules)
    events = tmp_path / "events.jsonl"
    os.environ["SUITE_MIN_EVENTS"] = str(events)
    yield events
    os.environ.clear()
    os.environ.update(environ)
    os.chdir(cwd)
    sys.path[:] = path
    for name in set(vars(builtins)) - set(names):
        delattr(builtins, name)
    vars(builtins).update(names)
    for name in set(sys.modules) - modules:
        del sys.modules[name]
    results.reset()
    squishinfo.testCase = squishinfo.resultDir = ""
    aut.configure({}, {})


@pytest.fixture(autouse=True)
def aut_unconfigured():
    """Fail a test that leaves the applications configured, as with a log dir.

    Autouse, so it is torn down after the test's own fixtures.
    """
    yield
    # pylint: disable=protected-access
    leftover = {"log dir": aut._LOG_DIR, "applications": aut._AUT_PATHS}
    assert not any(leftover.values()), f"aut still configured: {leftover}"
