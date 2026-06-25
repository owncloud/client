Feature: capture documentation screenshots
    As a documentation maintainer
    I want to regenerate the screenshots that ship in the desktop manual
    So that the docs stay in sync with the current client UI

    # This suite is run ON DEMAND only (not part of the regular GUI test run).
    # Each scenario navigates to a documented screen using the existing page
    # objects and then captures it with the screen id from
    # DocScreenshotHelper.SCREENSHOT_REGISTRY. Captures crop to the relevant
    # top-level window (see SCREENSHOT_WINDOW) so the images exclude desktop
    # chrome.
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
            | server | %local_server% |
        When the user adds the following user credentials:
            | user     | Alice |
            | password | 1234  |
        And the user opens the advanced configuration
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

    Scenario: Capture the General settings pane
        Given user "Alice" has set up a client with default settings
        When the user opens the settings tab
        Then the screenshot "settings-general" is captured

    Scenario: Capture the Ignored Files editor
        Given user "Alice" has set up a client with default settings
        When the user opens the ignored files editor
        Then the screenshot "ignored-files-editor" is captured

    Scenario: Capture the Log Output window
        Given user "Alice" has set up a client with default settings
        When the user opens the log settings window
        Then the screenshot "log-output-window" is captured

    Scenario: Capture the folder sync options menu
        Given user "Alice" has set up a client with default settings
        When the user opens the folder sync options menu
        Then the screenshot "folder-menu" is captured
