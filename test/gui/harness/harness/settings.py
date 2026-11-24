# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""The ``testSettings`` object shared by the shim modules."""


class TestSettings:  # pylint: disable=too-few-public-methods
    # Squish's attribute names, read and written by suite code.
    # pylint: disable=invalid-name
    def __init__(self):
        self.throwOnFailure = False
        self.waitForObjectTimeout = 20000


test_settings = TestSettings()
