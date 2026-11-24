# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Copies of the suite_min fixture with an extra test case."""

import shutil
from pathlib import Path

SUITE_MIN = Path(__file__).resolve().parent / "data" / "suite_min"

FAILING_END_HOOKS = """

@OnFeatureEnd
def hook(context):
    raise RuntimeError('first end hook broke')


@OnFeatureEnd
def hook(context):
    raise RuntimeError('second end hook broke')
"""


def copy_suite(tmp_path, feature, end_hooks=False):
    """suite_min with an extra test case tst_b running the given feature.

    With ``end_hooks`` the suite also gets two OnFeatureEnd hooks that raise.
    """
    target = tmp_path / "suite"
    shutil.copytree(SUITE_MIN, target)
    shutil.copytree(target / "tst_a", target / "tst_b")
    (target / "tst_b" / "test.feature").write_text(feature, encoding="utf-8")
    if end_hooks:
        hooks = target / "shared" / "scripts" / "bdd_hooks.py"
        hooks.write_text(hooks.read_text("utf-8") + FAILING_END_HOOKS, "utf-8")
    return target
