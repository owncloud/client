# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
import json
import os


def record_event(*parts):
    """Append one event to the file named by SUITE_MIN_EVENTS, if it is set."""
    if path := os.environ.get('SUITE_MIN_EVENTS'):
        with open(path, 'a', encoding='utf-8') as f:
            f.write(json.dumps(list(parts)) + '\n')
