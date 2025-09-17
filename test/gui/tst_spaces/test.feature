@skipOnOC10
Feature: Project spaces
    As a user
    I want to sync project space
    So that I can do view and manage the space

    Background:
        Given user "Alice" has been created in the server with default attributes
        And the administrator has created a space "Project101"


    Scenario: User with Viewer role can open the file
        Given the administrator has created a folder "planning" in space "Project101"
        And the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "viewer"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        And the user waits for file "testfile.txt" to be synced
        Then user "Alice" should be able to open the file "testfile.txt" on the file system
        And as "Alice" the file "testfile.txt" should have content "some content" on the file system

    @skipOnWindows
    Scenario: User with Viewer role cannot edit the file (Linux only)
        Given the administrator has created a folder "planning" in space "Project101"
        And the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "viewer"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        And the user waits for file "testfile.txt" to be synced
        Then user "Alice" should not be able to edit the file "testfile.txt" on the file system
        And as "Alice" the file "testfile.txt" in the space "Project101" should have content "some content" in the server

    @skipOnLinux
    Scenario: User with Viewer role cannot edit the file (Windows only)
        Given the administrator has created a folder "planning" in space "Project101"
        And the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "viewer"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        And the user waits for file "testfile.txt" to be synced
        And the user overwrites the file "testfile.txt" with content "overwrite some content"
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "testfile.txt" should be blacklisted
        And as "Alice" the file "testfile.txt" in the space "Project101" should have content "some content" in the server


    Scenario: User with Editor role can edit the file
        Given the administrator has created a folder "planning" in space "Project101"
        And the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "editor"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        And the user waits for file "testfile.txt" to be synced
        And the user overwrites the file "testfile.txt" with content "some content edited"
        And the user waits for file "testfile.txt" to be synced
        Then as "Alice" the file "testfile.txt" in the space "Project101" should have content "some content edited" in the server


    Scenario: User with Manager role can add files and folders
        Given the administrator has added user "Alice" to space "Project101" with role "manager"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        And user "Alice" creates a file "localFile.txt" with the following content inside the sync folder
            """
            test content
            """
        And user "Alice" creates a folder "localFolder" inside the sync folder
        And the user waits for the files to sync
        Then as "Alice" the file "localFile.txt" in the space "Project101" should have content "test content" in the server
        And as "Alice" the space "Project101" should have folder "localFolder" in the server

    @predefined_users @skip
    Scenario: User with viewer role cannot sync space to the desktop client
        Given the administrator has added user "Alice" to space "Project101" with role "viewer"
        And the user has started the client
        When the user adds the following account:
            | server   | %local_server% |
            | user     | Alice          |
            | password | 1234           |
        Then the space "Project101" should not exist in the sync folder list

    @predefined_users @skip
    Scenario: User with downloader role cannot add or delete file
        Given the administrator has created a folder "planning" in space "Project101"
        And the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "downloader"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        Then user "Alice" should be able to open the file "testfile.txt" on the file system
        And as "Alice" the file "testfile.txt" should have content "some content" on the file system
        When user "Alice" creates a file "lorem.txt" with the following content inside the sync folder
            """
            test content
            """
        And user "Alice" creates a folder "localFolder" inside the sync folder
        And the user deletes the file "testfile.txt"
        And the user deletes the folder "planning"
        Then as "Alice" the space "Project101" should not have file "lorem.txt" in the server
        And as "Alice" the space "Project101" should not have folder "localFolder" in the server
        And as "Alice" the space "Project101" should have file "testfile.txt" in the server
        And as "Alice" the space "Project101" should have file "planning" in the server

    @predefined_users @skip
    Scenario: User with uploader role can edit/upload files
        Given the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "uploader"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        Then the file "testfile.txt" should not exist on the file system
        And the folder "planning" should not exist on the file system
        When the user overwrites the file "testfile.txt" with content "some content edited"
        And the user waits for file "testfile.txt" to be synced
        Then as "Alice" the file "testfile.txt" in the space "Project101" should have content "some content edited" in the server
        When user "Alice" creates a file "lorem.txt" with the following content inside the sync folder
            """
            test content
            """
        And the user waits for file "lorem.txt" to be synced
        Then as "Alice" the file "lorem.txt" in the space "Project101" should have content "test content" in the server

    @predefined_users @skip
    Scenario Outline: User with collaborator and manager role can add, edit and delete files and folders in space
        Given the administrator has uploaded a file "testfile.txt" with content "some content" inside space "Project101"
        And the administrator has created a folder "planning" in space "Project101"
        And the administrator has added user "Alice" to space "Project101" with role "<role>"
        And user "Alice" has set up a client with default settings
        When using sync connection folder "Project101"
        Then user "Alice" should be able to open the file "testfile.txt" on the file system
        And as "Alice" the file "testfile.txt" should have content "some content" on the file system
        When the user overwrites the file "testfile.txt" with content "some content edited"
        And the user waits for file "testfile.txt" to be synced
        Then as "Alice" the file "testfile.txt" in the space "Project101" should have content "some content edited" in the server
        When user "Alice" creates a file "lorem.txt" with the following content inside the sync folder
            """
            test content
            """
        And user "Alice" creates a folder "localFolder" inside the sync folder
        And the user waits for the files to sync
        Then as "Alice" the file "lorem.txt" in the space "Project101" should have content "test content" in the server
        And as "Alice" the space "Project101" should have folder "localFolder" in the server
        When the user deletes the file "testfile.txt"
        And the user deletes the folder "planning"
        And the user waits for the files to sync
        Then as "Alice" the space "Project101" should not have file "testfile.txt" in the server
        And as "Alice" the space "Project101" should not have folder "planning" in the server
        Examples:
            | role         |
            | collaborator |
            | manager      |


