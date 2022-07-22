Feature: filter accounts

    As a user
    I want to filter the accounts
    So that I can view the data of the filtered accounts

    Scenario: synced files can be filter based on user account
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "simple-folder" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "simple-folder" to be synced
        And the user has added another account with
            | server   | %local_server% |
            | user     | Brian          |
            | password | AaBb2Cc3Dd4    |
        When the user clicks on the activity tab
        And the user selects "Alice Hansen@localhost" option on filter dropdown button
        Then the following information should be displayed on the sync table
            |action    |resource		   |account                 |
            |Downloaded|simple-folder      |Alice Hansen@localhost  |

	Scenario: Filter not synced file
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And user "Alice" has set up a client with default settings
        And user "Alice" has created a folder "Folder1" inside the sync folder
        And the user has waited for folder "Folder1" to be synced
        And user "Alice" has created the following files inside the sync folder:
            | files             |
            | /.htaccess        |
            | /Folder1/a\\a.txt |
        When the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "Folder1/a\\a.txt" should be blacklisted
        And the file ".htaccess" should be excluded
        When the user uncheck "Excluded" filter on not synced tab
        Then the file ".htaccess" should not be visible



