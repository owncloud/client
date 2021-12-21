Feature: Enable/disable virtual file support

    As a user
    I want to enable virtual file support
    So that I can synchronize virtual files with local folder

    @skip
    Scenario: Enable VFS
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And user "Alice" has set up a client with default settings
        When the user enables virtual file support
        Then the "Disable virtual file support..." button should be available

    @skip
    Scenario: Disable VFS
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And user "Alice" has set up a client with default settings
        And the user has enabled virtual file support
        When the user disables virtual file support
        Then the "Enable virtual file support (experimental)..." button should be available