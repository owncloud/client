# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
Feature: minimal suite
    As a maintainer of the runner
    I want a small suite in the shapes of a real one
    So that the runner's semantics are tested without an application

    Background:
        Given user "Alice" has been created

    @smokeTest
    Scenario: tables, doc strings and user data
        Given the user has set up the following accounts:
            | user  | server       |
            | Alice | example.test |
        When user "Alice" creates a file "notes.txt" with the following content
            """
            first line
              second line
            """
        And the user waits for file "notes.txt" to be synced
        And the user remembers "kept"
        Then the remembered value should be "kept"
        And the word notes should have 5 letters

    @crash
    Scenario: the application crashes
        When the application crashes
        Then the remembered value should be "never"


    Scenario Outline: syncing after a crash
        When the user waits for folder "<name>" to be synced
        Then nothing should be remembered
        Examples:
            | name |
            | one  |
            | two  |

    @skip
    Scenario: skipped by tag
        When the user waits for file "skipped.txt" to be synced
