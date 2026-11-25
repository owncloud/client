# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's ``squishinfo`` module; the runner sets both values.

``resultDir`` is set when the run starts, ``testCase`` before each test case.
"""

# Squish's attribute names.
# pylint: disable=invalid-name

# Absolute path of the test case (``tst_*`` directory) that is running.
testCase = ""
# Directory the HTML report is written to (the first ``--reportgen html,DIR``),
# or "" when there is none.
resultDir = ""
