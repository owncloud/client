# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
from helpers.EventHelper import record_event
from pkg.sub.mod import greeting


@Given('user "|any|" has been created')
def step(context, username):
    record_event('step', f'created {username}', greeting())


@Given('the user has set up the following accounts:')
def step(context):
    record_event('table', context.table)


@When('user "|any|" creates a file "|any|" with the following content')
def step(context, username, filename):
    record_event('docstring', username, filename, '\n'.join(context.multiLineText))


@When(r'the user waits for (file|folder) "([^"]*)" to be synced', regexp=True)
def step(context, resource_type, resource):
    record_event('step', f'synced {resource_type} {resource}')


@Step('the user remembers "|any|"')
def step(context, value):
    context.userData = value


@Then('the remembered value should be "|any|"')
def step(context, expected):
    test.compare(context.userData, expected, 'remembered value')


@Then('nothing should be remembered')
def step(context):
    test.verify(context.userData is None, 'nothing remembered')


@Then('the word |word| should have |integer| letters')
def step(context, word, count):
    record_event('step', word, count)
    test.compare(len(word), count, 'letters')


@When('the application crashes')
def step(context):
    # stands in for an exception from a crashed application under test
    raise RuntimeError('the application under test crashed')
