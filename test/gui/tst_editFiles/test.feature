Feature: edit files

    As a user
    I want to be able to edit the file content
    So that I can modify and change file data


    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files


    Scenario Outline: Modify orignal content of a file
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "<filename>" on the server
        And user "Alice" has set up a client with default settings
        When the user overwrites the file "<filename>" with content "overwrite ownCloud test text file"
        Then as "Alice" the file "<filename>" on the server should have the content "overwrite ownCloud test text file"
        Examples:
            | filename     |
            | textfile.txt |
            | textfile.doc |
            | textfile.xls |
            | textfile.pdf |