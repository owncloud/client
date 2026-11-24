# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
import os

from helpers.EventHelper import record_event
from pkg.sub import mod

testSettings.throwOnFailure = True

PREVIOUS_FAIL_RESULT_COUNT = 0
PREVIOUS_ERROR_RESULT_COUNT = 0


# Order: 1
@OnFeatureStart
def hook(context):
    record_event('OnFeatureStart', context.title)
    record_event(
        'setup',
        os.path.basename(os.getcwd()),
        os.environ.get('SUITE_MIN_GREETING'),
        callable(object.children),
    )
    record_event('hooks', mod.greeting())


@OnScenarioStart
def hook(context):
    record_event('OnScenarioStart', context.title)


@OnStepStart
def hook(context):
    record_event('OnStepStart', context.title)


@OnStepEnd
def hook(context):
    record_event('OnStepEnd', context.title)


def scenario_failed():
    return (
        test.resultCount('fails') - PREVIOUS_FAIL_RESULT_COUNT > 0
        or test.resultCount('errors') - PREVIOUS_ERROR_RESULT_COUNT > 0
    )


# Order: 1
@OnScenarioEnd
def hook(context):
    record_event('OnScenarioEnd', context.title, scenario_failed())
    global PREVIOUS_FAIL_RESULT_COUNT, PREVIOUS_ERROR_RESULT_COUNT
    PREVIOUS_FAIL_RESULT_COUNT = test.resultCount('fails')
    PREVIOUS_ERROR_RESULT_COUNT = test.resultCount('errors')


# Order: 2
@OnScenarioEnd
def hook(context):
    record_event('OnScenarioEnd:2', context.title)


@OnFeatureEnd
def hook(context):
    record_event('OnFeatureEnd', context.title)
