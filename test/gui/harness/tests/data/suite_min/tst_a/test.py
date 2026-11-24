# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
# Squish puts these functions into builtins.
# pylint: disable=undefined-variable
source(findFile('scripts', 'python/bdd.py'))

setupHooks('../shared/scripts/bdd_hooks.py')
collectStepDefinitions('./steps', '../shared/steps')


def main():
    runFeatureFile('test.feature')
