# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
# pkg and pkg.sub are namespace packages: neither has an __init__.py.
from helpers.EventHelper import record_event

record_event('imported', __name__)


def greeting():
    return 'hello from a namespace package'
