Feature: .zip files sync

    As a user
    I want to be able to upload zip file into the sync root
    So that extracted files and folders are available on the server


    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files


    Scenario: Syncing a zip file to the server
        Given the user has created a zip file "zippedfileAndfolders.zip" with following folders and files
            |resource  |type   	|content 	|
            |folder1   |folder  |			|
            |folder2   |folder 	|			|
            |file1.txt |file	|Test file1 |
            |file2.txt |file	|Test file2 |
        And user "Alice" has set up a client with default settings
        When user "Alice" moves file "zippedfileAndfolders.zip" from the temp folder into the sync folder
		And the user "Alice" unzips the zip file "zippedfileAndfolders.zip" inside the sync root
		And the user waits for the files to sync
		Then as "Alice" folder "folder1" should exist on the server
        And as "Alice" folder "folder2" should exist on the server
        And as "Alice" file "file1.txt" should exist on the server
        And as "Alice" file "file2.txt" should exist on the server
        And the content of file "file1.txt" for user "Alice" should be "Test file1" on the server
        And the content of file "file2.txt" for user "Alice" should be "Test file2" on the server
