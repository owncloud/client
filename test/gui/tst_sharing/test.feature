@skipOnOCIS @pr-10241
Feature: Sharing
    As a user
    I want to share files and folders with other users
    So that those users can access the files and folders

    Background:
        Given user "Alice" has been created in the server with default attributes

    @smokeTest
    Scenario: simple sharing with user
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user adds "Brian Murphy" as collaborator of resource "textfile0.txt" with permissions "edit,share" using the client-UI
        And the user adds "Brian Murphy" as collaborator of resource "simple-folder" with permissions "edit,share" using the client-UI
        Then user "Brian Murphy" should be listed in the collaborators list for file "textfile0.txt" with permissions "edit,share" on the client-UI
        And user "Brian Murphy" should be listed in the collaborators list for file "simple-folder" with permissions "edit,share" on the client-UI
        And as "Brian" folder "simple-folder" should exist in the server
        And as "Brian" file "textfile0.txt" should exist in the server


    Scenario: sharing file and folder with user who has some other shares
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has created folder "shared" in the server
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has uploaded file with content "shared file" to "sharedfile.txt" in the server
        And user "Alice" has shared folder "shared" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "sharedfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has set up a client with default settings
        When the user adds "Brian Murphy" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        And the user adds "Brian Murphy" as collaborator of resource "simple-folder" with permissions "edit,share" using the client-UI
        Then user "Brian Murphy" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And user "Brian Murphy" should be listed in the collaborators list for file "simple-folder" with permissions "edit,share" on the client-UI
        And as "Brian" folder "shared" should exist in the server
        And as "Brian" file "sharedfile.txt" should exist in the server
        And as "Brian" folder "simple-folder" should exist in the server
        And as "Brian" file "textfile.txt" should exist in the server


    Scenario: sharing file/folder with a user that has special characters as username
        Given user "Speci@l_Name-.+" has been created in the server with default attributes
        And user "Alice" has created folder "FOLDER" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        And the user adds "Speci@l_Name-.+" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        And the user adds "Speci@l_Name-.+" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        Then user "Speci@l_Name-.+" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And user "Speci@l_Name-.+" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And as "Speci@l_Name-.+" folder "FOLDER" should exist in the server
        And as "Speci@l_Name-.+" file "textfile.txt" should exist in the server


    Scenario: Share files/folders with special characters in their name
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has created folder "SampleFolder,With,$pecial#Characters" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/$ample1#.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user adds "Brian Murphy" as collaborator of resource "SampleFolder,With,$pecial#Characters" with permissions "edit,share" using the client-UI
        And the user adds "Brian Murphy" as collaborator of resource "$ample1#.txt" with permissions "edit,share" using the client-UI
        Then user "Brian Murphy" should be listed in the collaborators list for file "SampleFolder,With,$pecial#Characters" with permissions "edit,share" on the client-UI
        And user "Brian Murphy" should be listed in the collaborators list for file "$ample1#.txt" with permissions "edit,share" on the client-UI
        And as "Brian" folder "SampleFolder,With,$pecial#Characters" should exist in the server
        And as "Brian" file "$ample1#.txt" should exist in the server


    Scenario: try to share a file/folder with a user to whom the file has already been shared
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has created folder "SharedFolder" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "/textfile.txt" in the server
        And user "Alice" has shared folder "SharedFolder" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        And the user searches for collaborator "Brian Murphy" using the client-UI
        Then the error "No results for 'Brian Murphy'" should be displayed in the sharing dialog
        When the user closes the sharing dialog
        And the user opens the sharing dialog of "SharedFolder" using the client-UI
        And the user searches for collaborator "Brian Murphy" using the client-UI
        Then the error "No results for 'Brian Murphy'" should be displayed in the sharing dialog
        And the user closes the sharing dialog


    Scenario: try to self share a file/folder
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "/textfile.txt" in the server
        And user "Alice" has created folder "OwnFolder" in the server
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        And the user searches for collaborator with autocomplete characters "Alice Hansen" using the client-UI
        Then the error "No results for 'Alice Hansen'" should be displayed in the sharing dialog
        When the user closes the sharing dialog
        And the user opens the sharing dialog of "OwnFolder" using the client-UI
        And the user searches for collaborator with autocomplete characters "Alice Hansen" using the client-UI
        Then the error "No results for 'Alice Hansen'" should be displayed in the sharing dialog
        And the user closes the sharing dialog


    Scenario: search for users with minimum autocomplete characters
        Given user "TestUser1" has been created in the server with default attributes
        And user "TestUser2" has been created in the server with default attributes
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        And the user searches for collaborator with autocomplete characters "Tes" using the client-UI
        Then the following users should be listed as suggested collaborators:
            | user      |
            | TestUser1 |
            | TestUser2 |
        And the user closes the sharing dialog


    Scenario: autocomplete offers a list of users followed by a list of groups
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        And the user searches for collaborator with autocomplete characters "Adm" using the client-UI
        Then the following users should be listed as suggested collaborators:
            | user          |
            | admin         |
            | admin (group) |
        And the user closes the sharing dialog


    Scenario: collaborators are listed in chronological order
        Given user "Brian" has been created in the server with default attributes
        And user "Carol" has been created in the server with default attributes
        And user "TestUser1" has been created in the server with default attributes
        And user "TestUser2" has been created in the server with default attributes
        And user "TestUser3" has been created in the server with default attributes
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has shared file "textfile.txt" in the server with user "Carol" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "TestUser1" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "TestUser3" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "TestUser2" with "all" permissions
        And user "Alice" has set up a client with default settings
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        Then the collaborators should be listed in the following order:
            | collaborator |
            | Carol King   |
            | Brian Murphy |
            | TestUser1    |
            | TestUser3    |
            | TestUser2    |
        And the user closes the sharing dialog

    @issue-7459
    Scenario: Progress indicator should not be visible after unselecting the password protection checkbox while sharing through public link
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user opens the public links dialog of "textfile0.txt" using the client-UI
        And the user toggles the password protection using the client-UI
        And the user toggles the password protection using the client-UI
        Then the password progress indicator should not be visible in the client-UI - expected to fail
        And the user closes the sharing dialog


    Scenario: Collaborator should not see to whom a file/folder is shared.
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created folder "Folder" in the server
        And user "Alice" has shared file "/textfile0.txt" in the server with user "Brian" with "read, share" permission
        And user "Alice" has shared folder "Folder" in the server with user "Brian" with "read, share" permission
        And user "Brian" has set up a client with default settings
        When the user opens the sharing dialog of "textfile0.txt" using the client-UI
        Then the error text "The item is not shared with any users or groups" should be displayed in the sharing dialog
        When the user closes the sharing dialog
        And the user opens the sharing dialog of "Folder" using the client-UI
        Then the error text "The item is not shared with any users or groups" should be displayed in the sharing dialog
        And the user closes the sharing dialog


    Scenario: share file and folder to a group
        Given group "grp1" has been created in the server
        And user "Brian" has been created in the server with default attributes
        And user "Brian" has been added to group "grp1" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has set up a client with default settings
        When the user adds group "grp1" as collaborator of resource "textfile0.txt" with permissions "edit,share" using the client-UI
        Then group "grp1" should be listed in the collaborators list for file "textfile0.txt" with permissions "edit,share" on the client-UI
        When the user adds group "grp1" as collaborator of resource "simple-folder" with permissions "edit,share" using the client-UI
        Then group "grp1" should be listed in the collaborators list for file "simple-folder" with permissions "edit,share" on the client-UI
        And as "Brian" folder "simple-folder" should exist in the server
        And as "Brian" file "textfile0.txt" should exist in the server


    Scenario: User (non-author) can not share to a group to which the file/folder is already shared
        Given user "Brian" has been created in the server with default attributes
        And group "grp1" has been created in the server
        And user "Brian" has been added to group "grp1" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created folder "Folder" in the server
        And user "Alice" has shared file "/textfile0.txt" in the server with user "Brian" with "read, share, update" permission
        And user "Alice" has shared folder "Folder" in the server with user "Brian" with "read, share, update" permission
        And user "Alice" has shared file "/textfile0.txt" in the server with group "grp1" with "read, share, update" permission
        And user "Alice" has shared folder "Folder" in the server with group "grp1" with "read, share, update" permission
        And user "Brian" has set up a client with default settings
        When the user tires to share resource "textfile0.txt" with the group "grp1" using the client-UI
        Then the error "Path already shared with this group" should be displayed in the sharing dialog
        When the user closes the sharing dialog
        And the user tires to share resource "Folder" with the group "grp1" using the client-UI
        Then the error "Path already shared with this group" should be displayed in the sharing dialog
        And the user closes the sharing dialog


    Scenario: sharee edits content of files shared by sharer
        Given user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has uploaded file with content "file inside a folder" to "simple-folder/textfile.txt" in the server
        And user "Alice" has uploaded file with content "file in the root" to "textfile.txt" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared folder "simple-folder" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user overwrites the file "textfile.txt" with content "overwrite file in the root"
        And the user waits for file "textfile.txt" to be synced
        And the user overwrites the file "simple-folder/textfile.txt" with content "overwrite file inside a folder"
        And the user waits for file "simple-folder/textfile.txt" to be synced
        Then as "Brian" the file "simple-folder/textfile.txt" should have the content "overwrite file inside a folder" in the server
        And as "Brian" the file "textfile.txt" should have the content "overwrite file in the root" in the server
        And as "Alice" the file "simple-folder/textfile.txt" should have the content "overwrite file inside a folder" in the server
        And as "Alice" the file "textfile.txt" should have the content "overwrite file in the root" in the server


    Scenario: sharee tries to edit content of files shared without write permission
        Given user "Alice" has created folder "Parent" in the server
        And user "Alice" has uploaded file with content "file inside a folder" to "Parent/textfile.txt" in the server
        And user "Alice" has uploaded file with content "file in the root" to "textfile.txt" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared folder "Parent" in the server with user "Brian" with "read" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "read" permissions
        And user "Brian" has set up a client with default settings
        When the user tries to overwrite the file "Parent/textfile.txt" with content "overwrite file inside a folder"
        And the user tries to overwrite the file "textfile.txt" with content "overwrite file in the root"
        And the user waits for file "textfile.txt" to have sync error
        Then as "Brian" the file "Parent/textfile.txt" should have the content "file inside a folder" in the server
        And as "Brian" the file "textfile.txt" should have the content "file in the root" in the server
        And as "Alice" the file "Parent/textfile.txt" should have the content "file inside a folder" in the server
        And as "Alice" the file "textfile.txt" should have the content "file in the root" in the server


    Scenario: sharee edits shared files and again try to edit after write permission is revoked
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has created folder "FOLDER" in the server
        And user "Alice" has uploaded file with content "some content" to "FOLDER/simple.txt" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared folder "FOLDER" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user opens the add-account dialog
        And the user adds the following account:
            | server   | %local_server% |
            | user     | Alice          |
            | password | 1234           |
        And the user removes permissions "edit" for user "Brian Murphy" of resource "textfile.txt" using the client-UI
        And the user closes the sharing dialog
        And the user removes permissions "edit" for user "Brian Murphy" of resource "FOLDER" using the client-UI
        And the user closes the sharing dialog
        And user "Brian" tries to overwrite the file "textfile.txt" with content "overwrite ownCloud test text file"
        And user "Brian" tries to overwrite the file "FOLDER/simple.txt" with content "overwrite some content"
        And user "Brian" waits for file "textfile.txt" to have sync error
        Then as "Brian" the file "textfile.txt" should have the content "ownCloud test text file" in the server
        And as "Brian" the file "FOLDER/simple.txt" should have the content "some content" in the server
        And as "Alice" the file "textfile.txt" should have the content "ownCloud test text file" in the server
        And as "Alice" the file "FOLDER/simple.txt" should have the content "some content" in the server


    Scenario: sharee creates a file and a folder inside a shared folder
        Given user "Alice" has created folder "Parent" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared folder "Parent" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When user "Brian" creates a file "Parent/localFile.txt" with the following content inside the sync folder
            """
            test content
            """
        And user "Brian" creates a folder "Parent/localFolder" inside the sync folder
        And the user waits for file "Parent/localFile.txt" to be synced
        And the user waits for folder "Parent/localFolder" to be synced
        Then as "Brian" file "Parent/localFile.txt" should exist in the server
        And as "Brian" folder "Parent/localFolder" should exist in the server
        And as "Alice" file "Parent/localFile.txt" should exist in the server
        And as "Alice" folder "Parent/localFolder" should exist in the server


    Scenario: sharee tries to create a file and a folder inside a shared folder without write permission
        Given user "Alice" has created folder "Parent" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared folder "Parent" in the server with user "Brian" with "read" permissions
        And user "Brian" has set up a client with default settings
        When user "Brian" creates a file "Parent/localFile.txt" with the following content inside the sync folder
            """
            test content
            """
        And user "Brian" creates a folder "Parent/localFolder" inside the sync folder
        And the user waits for file "Parent/localFile.txt" to have sync error
        And the user waits for folder "Parent/localFolder" to have sync error
        Then as "Brian" file "Parent/localFile.txt" should not exist in the server
        And as "Brian" folder "Parent/localFolder" should not exist in the server
        And as "Alice" file "Parent/localFile.txt" should not exist in the server
        And as "Alice" folder "Parent/localFolder" should not exist in the server


    Scenario: sharee renames the shared file and folder
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "textfile.txt" in the server
        And user "Alice" has created folder "FOLDER" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "FOLDER" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user renames a file "textfile.txt" to "lorem.txt"
        And the user renames a folder "FOLDER" to "PARENT"
        And the user waits for folder "PARENT" to be synced
        And the user waits for file "lorem.txt" to be synced
        Then as "Brian" folder "FOLDER" should not exist in the server
        And as "Brian" file "textfile.txt" should not exist in the server
        And as "Brian" folder "PARENT" should exist in the server
        And as "Brian" file "lorem.txt" should exist in the server
        # File/folder will not change for Alice
        And as "Alice" folder "FOLDER" should exist in the server
        And as "Alice" file "textfile.txt" should exist in the server
        And as "Alice" folder "PARENT" should not exist in the server
        And as "Alice" file "lorem.txt" should not exist in the server

    @issue-9439 @issue-11102
    Scenario: sharee deletes a file and folder shared by sharer
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "textfile.txt" in the server
        And user "Alice" has created folder "Folder" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "Folder" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user deletes the file "textfile.txt"
        And the user deletes the folder "Folder"
        And the user waits for the files to sync
        Then as "Brian" file "textfile.txt" should not exist in the server
        And as "Brian" folder "Folder" should not exist in the server
        And as "Alice" file "textfile.txt" should exist in the server
        And as "Alice" folder "Folder" should exist in the server

    @issue-11102
    Scenario: sharee tries to delete shared file and folder without permissions
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "textfile.txt" in the server
        And user "Alice" has created folder "Folder" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "read" permissions
        And user "Alice" has shared file "Folder" in the server with user "Brian" with "read" permissions
        And user "Brian" has set up a client with default settings
        When the user deletes the file "textfile.txt"
        And the user deletes the folder "Folder"
        And the user waits for the files to sync
        # Sharee can delete (means unshare) the file shared with read permission
        Then as "Brian" file "textfile.txt" should not exist in the server
        And as "Brian" folder "Folder" should not exist in the server
        And as "Alice" file "textfile.txt" should exist in the server
        And as "Alice" folder "Folder" should exist in the server


    Scenario: reshare a file/folder
        Given user "Brian" has been created in the server with default attributes
        And user "Carol" has been created in the server with default attributes
        And user "Alice" has created folder "FOLDER" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has shared file "FOLDER" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user adds "Carol King" as collaborator of resource "FOLDER" with permissions "edit,share" using the client-UI
        And the user adds "Carol King" as collaborator of resource "textfile.txt" with permissions "edit,share" using the client-UI
        Then user "Carol King" should be listed in the collaborators list for file "FOLDER" with permissions "edit,share" on the client-UI
        And user "Carol King" should be listed in the collaborators list for file "textfile.txt" with permissions "edit,share" on the client-UI
        And as "Carol" folder "FOLDER" should exist in the server
        And as "Carol" file "textfile.txt" should exist in the server


    Scenario: try to reshare a file/folder shared without share permission
        Given user "Brian" has been created in the server with default attributes
        And user "Carol" has been created in the server with default attributes
        And user "Alice" has created folder "FOLDER" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Alice" has shared file "FOLDER" in the server with user "Brian" with "read" permissions
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "read" permissions
        And user "Brian" has set up a client with default settings
        When the user opens the sharing dialog of "FOLDER" using the client-UI
        Then the error text "The file can not be shared because it was shared without sharing permission." should be displayed in the sharing dialog
        And the user closes the sharing dialog
        When the user opens the sharing dialog of "textfile.txt" using the client-UI
        Then the error text "The file can not be shared because it was shared without sharing permission." should be displayed in the sharing dialog
        And the user closes the sharing dialog


    Scenario: unshare a shared file and folder
        Given user "Brian" has been created in the server with default attributes
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "textfile0.txt" in the server
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has shared file "textfile0.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared folder "simple-folder" in the server with user "Brian" with "all" permissions
        And user "Alice" has set up a client with default settings
        When the user unshares the resource "textfile0.txt" for collaborator "Brian Murphy" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog
        And as "Brian" file "textfile0.txt" should not exist in the server
        When the user closes the sharing dialog
        And the user unshares the resource "simple-folder" for collaborator "Brian Murphy" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog
        And the user closes the sharing dialog
        And as "Brian" folder "simple-folder" should not exist in the server


    Scenario: share a file with many users
        Given user "Brian" has been created in the server with default attributes
        And user "Carol" has been created in the server with default attributes
        And user "David" has been created in the server with default attributes
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user adds following collaborators of resource "textfile0.txt" using the client-UI
            | user         | permissions |
            | Brian Murphy | edit,share  |
            | Carol King   | edit,share  |
            | David Lopez  | edit,share  |
        Then the following users should be listed in as collaborators for file "textfile0.txt" on the client-UI
            | user         | permissions |
            | Brian Murphy | edit,share  |
            | Carol King   | edit,share  |
            | David Lopez  | edit,share  |

    @issue-7423
    Scenario: unshare a reshared file
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "textfile.txt" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Carol" has been created in the server with default attributes
        And user "Alice" has shared file "textfile.txt" in the server with user "Brian" with "all" permissions
        And user "Brian" has shared file "textfile.txt" in the server with user "Carol" with "all" permissions
        And user "Brian" has set up a client with default settings
        When the user unshares the resource "textfile.txt" for collaborator "Carol King" using the client-UI
        Then the text "The item is not shared with any users or groups" should be displayed in the sharing dialog
        And the user closes the sharing dialog

    @smokeTest
    Scenario: simple sharing of file and folder by public link without password
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has created folder "simple-folder/child" in the server
        And user "Alice" has set up a client with default settings
        When the user creates a new public link for file "textfile0.txt" without password using the client-UI
        And the user closes the sharing dialog
        Then as user "Alice" the file "textfile0.txt" should have a public link in the server
        And the public should be able to download the file "textfile0.txt" without password from the last created public link by "Alice" in the server
        When the user creates a new public link with permissions "Download / View" for folder "simple-folder" without password using the client-UI
        Then as user "Alice" the folder "simple-folder" should have a public link in the server
        And the user closes the sharing dialog


    Scenario Outline: simple sharing of file and folder by public link with password
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has set up a client with default settings
        When the user creates a new public link for file "textfile0.txt" with password "<password>" using the client-UI
        And the user closes the sharing dialog
        Then as user "Alice" the file "textfile0.txt" should have a public link in the server
        And the public should be able to download the file "textfile0.txt" with password "<password>" from the last created public link by "Alice" in the server
        When the user creates a new public link with permissions "Download / View" for folder "simple-folder" with password "<password>" using the client-UI
        Then as user "Alice" the folder "simple-folder" should have a public link in the server
        And the user closes the sharing dialog
        Examples:
            | password     |
            | password1234 |
            | p@$s!23      |


    Scenario: sharing of a file by public link and deleting the link
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created the following public link share in the server
            | resource    | textfile0.txt |
            | permissions | read          |
            | name        | Public-link   |
        And user "Alice" has set up a client with default settings
        When the user deletes the public link for file "textfile0.txt"
        And the user closes the sharing dialog
        Then as user "Alice" the file "/textfile0.txt" should not have any public link in the server


    Scenario: sharing of a file by public link with password and changing the password
        Given user "Alice" has uploaded file with content "ownCloud test text file 0" to "/textfile0.txt" in the server
        And user "Alice" has created the following public link share in the server
            | resource    | textfile0.txt |
            | permissions | read          |
            | name        | Public-link   |
            | password    | 1234          |
        And user "Alice" has set up a client with default settings
        When the user opens the public links dialog of "textfile0.txt" using the client-UI
        And the user changes the password of public link "Public-link" to "password1234" using the client-UI
        Then as user "Alice" the file "textfile0.txt" should have a public link in the server
        And the public should be able to download the file "textfile0.txt" with password "password1234" from the last created public link by "Alice" in the server
        And the user closes the sharing dialog


    Scenario: simple sharing of a file by public link with default expiration date
        Given user "Alice" has uploaded file with content "ownCloud test text file" to "/textfile.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user creates a new public link with following settings using the client-UI:
            | path       | textfile.txt |
            | expireDate | %default%    |
        And the user closes the sharing dialog
        Then the expiration date of the last public link of file "textfile.txt" should be "%default%"
        And as user "Alice" the file "textfile.txt" should have a public link in the server


    Scenario Outline: simple sharing of folder by public link with different roles
        Given user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has set up a client with default settings
        When the user creates a new public link for folder "simple-folder" using the client-UI with these details:
            | role | <role> |
        And the user closes the sharing dialog
        Then user "Alice" should have a share with these details in the server:
            | field       | value          |
            | share_type  | public_link    |
            | uid_owner   | Alice          |
            | permissions | <permissions>  |
            | path        | /simple-folder |
            | item_type   | folder         |
        Examples:
            | role        | permissions                  |
            | Viewer      | read                         |
            | Editor      | read, update, create, delete |
            | Contributor | create                       |


    Scenario: sharing a folder by public link with "Uploader" role and check if file can be downloaded
        Given user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "simple-folder/lorem.txt" in the server
        And user "Alice" has set up a client with default settings
        When the user creates a new public link for folder "simple-folder" using the client-UI with these details:
            | role | Contributor |
        And the user closes the sharing dialog
        Then user "Alice" should have a share with these details in the server:
            | field       | value          |
            | share_type  | public_link    |
            | uid_owner   | Alice          |
            | permissions | create         |
            | path        | /simple-folder |
            | item_type   | folder         |
        And the public should not be able to download the file "lorem.txt" from the last created public link by "Alice" in the server


    Scenario Outline: change collaborator permissions of a file & folder
        Given user "Alice" has created folder "simple-folder" in the server
        And user "Alice" has uploaded file with content "ownCloud test text file 0" to "lorem.txt" in the server
        And user "Brian" has been created in the server with default attributes
        And user "Alice" has shared folder "simple-folder" in the server with user "Brian" with "all" permissions
        And user "Alice" has shared file "lorem.txt" in the server with user "Brian" with "all" permissions
        And user "Alice" has set up a client with default settings
        When the user removes permissions "<permissions>" for user "Brian Murphy" of resource "simple-folder" using the client-UI
        Then "<permissions>" permissions should not be displayed for user "Brian Murphy" for resource "simple-folder" on the client-UI
        When the user closes the sharing dialog
        And the user removes permissions "<permissions>" for user "Brian Murphy" of resource "lorem.txt" using the client-UI
        Then "<permissions>" permissions should not be displayed for user "Brian Murphy" for resource "lorem.txt" on the client-UI
        And the user closes the sharing dialog
        And user "Alice" should have a share with these details in the server:
            | field       | value                        |
            | share_type  | user                         |
            | uid_owner   | Alice                        |
            | permissions | <expected-folder-permission> |
            | path        | /simple-folder               |
            | item_type   | folder                       |
            | share_with  | Brian                        |
        And user "Alice" should have a share with these details in the server:
            | field       | value                      |
            | share_type  | user                       |
            | uid_owner   | Alice                      |
            | permissions | <expected-file-permission> |
            | path        | /lorem.txt                 |
            | item_type   | file                       |
            | share_with  | Brian                      |
        Examples:
            | permissions | expected-folder-permission   | expected-file-permission |
            | edit        | read, share                  | read, share              |
            | share       | read, update, create, delete | read,update              |
            | edit,share  | read                         | read                     |
