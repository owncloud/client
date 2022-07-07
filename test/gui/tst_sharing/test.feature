Feature: Sharing

    As a user
    I want to share files and folders with other users
    So that those users can access the files and folders


    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And the setting "shareapi_auto_accept_share" on the server of app "core" has been set to "yes"


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has created folder "FOLDER" on the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Alice" has shared file "FOLDER" on the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" on the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist on the server
        And as "Carol" file "textfile.txt" should exist on the server


