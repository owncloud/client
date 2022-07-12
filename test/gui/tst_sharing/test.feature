@debug
Feature: Sharing

    As a user
    I want to share files and folders with other users
    So that those users can access the files and folders


    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files
        And the setting "shareapi_auto_accept_share" on the server of app "core" has been set to "yes"

    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog



    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog


    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" on the server
        And user "Brian" has been created on the server with default attributes and without skeleton files
        And user "Carol" has been created on the server with default attributes and without skeleton files
        And user "Alice" has shared folder "textfile.txt" on the server with user "Brian"
        And user "Brian" has shared folder "textfile.txt" on the server with user "Carol"
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog