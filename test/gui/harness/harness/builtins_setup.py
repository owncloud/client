# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's globals, which suite code uses without importing them."""

import builtins
import importlib

# Modules Squish provides as globals. ``object`` is one of them too, but in
# builtins it would replace Python's ``object`` for every module, so the
# runner puts it into the globals of the files it executes instead.
_MODULES = ("squish", "squishinfo", "test")


def install():
    """Put Squish's globals into builtins; the shim must be on sys.path."""
    for name in _MODULES:
        setattr(builtins, name, importlib.import_module(name))
    squish = importlib.import_module("squish")
    builtins.testSettings = squish.testSettings
    builtins.QApplication = squish.QApplication


def install_names(names):
    """Put further Squish globals (the BDD entry points) into builtins."""
    for name, value in names.items():
        setattr(builtins, name, value)
