# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Fixtures shared by the suite-running tests."""

import builtins
import os
import sys

import pytest

from harness import results


@pytest.fixture
def isolated(tmp_path):
    """Undo what a suite run changes in the process: env, cwd, paths, builtins."""
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
