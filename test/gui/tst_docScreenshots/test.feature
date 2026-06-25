Feature: capture documentation screenshots
    As a documentation maintainer
    I want to regenerate the screenshots that ship in the desktop manual
    So that the docs stay in sync with the current client UI

    # This suite is run ON DEMAND only (not part of the regular GUI test run).
    # Each scenario navigates to a documented screen using the existing page
    # objects and then captures it with the screen id from
    # DocScreenshotHelper.SCREENSHOT_REGISTRY.
    #
    # Surfaces that the Squish page objects cannot reach (OS file manager,
    # Windows VFS / Storage Sense, the MSI installer, the external browser and
    # the crash reporter) are captured manually per platform and are not part
    # of this suite.

    Background:
        Given user "Alice" has been created in the server with default attributes

    Scenario: Capture the account connection wizard - server URL page
        Given the user has started the client
        Then the screenshot "wizard-url" is captured

    Scenario: Capture the advanced configuration page
        Given the user has started the client
        And the user has entered the following account information:
            | server   | %local_server% |
            | user     | Alice          |
            | password | 1234           |
        When the user opens the advanced configuration
        Then the screenshot "wizard-all-set-advanced" is captured

    Scenario: Capture the account settings window
        Given user "Alice" has set up a client with default settings
        Then the screenshot "account-settings" is captured

    Scenario: Capture the spaces / sync connections list
        Given user "Alice" has set up a client with default settings
        Then the screenshot "spaces-show" is captured

    Scenario: Capture the Activity window
        Given user "Alice" has set up a client with default settings
        When the user clicks on the activity tab
        Then the screenshot "activity-pane" is captured
