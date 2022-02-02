Feature: deleting files and folders

  	As a user
  	I want to delete files and folders
  	So that I can keep my filing system clean and tidy


	Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files


    Scenario Outline: Delete a file
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "<fileName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for file "<fileName>" to be synced
        Then as "Alice" file "<fileName>" should exist on the server
        When the user deletes the file "<fileName>"
        And the user triggers force sync on the client
        And the user waits for file "<fileName>" to be synced
        Then as "Alice" file "<fileName>" should not exist on the server
        Examples:
            | fileName                                    |
            | textfile0.txt                               |
            | textfile0-with-name-more-than-20-characters |


    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for folder "<folderName>" to be synced
        Then as "Alice" folder "<folderName>" should exist on the server
        When the user deletes the folder "<folderName>"
        And the user triggers force sync on the client
        And the user waits for folder "<folderName>" to be synced
        Then as "Alice" folder "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
