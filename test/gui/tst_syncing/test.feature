@debug
Feature: Syncing files

    As a user
    I want to be able to sync my local folders to to my owncloud server
    so that I dont have to upload and download files manually

    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files

    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |


    Scenario Outline: Syncing a folder to the server
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a folder <foldername> inside the sync folder
        And the user waits for folder <foldername> to be synced
        Then as "Alice" folder <foldername> should exist on the server
        Examples:
            | foldername                                                               |
            | "myFolder"                                                               |
            | "really long folder name with some spaces and special char such as $%ñ&" |
            | "folder with space at end "                                              |