Feature: deleting files and folders

    As a user
    I want to delete files and folders
    So that I can keep my filing system clean and tidy


    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files

    # @issue-9439 @run-50
    # Scenario Outline: Delete a folder
    #     Given user "Alice" has created folder "<folderName>" on the server
    #     And user "Alice" has set up a client with default settings
    #     When the user waits for the files to sync
    #     And the user deletes the folder "<folderName>"
    #     And the user waits for the delete activity to sync
    #     Then as "Alice" file "<folderName>" should not exist on the server
    #     Examples:
    #         | folderName                                      |
    #         | simple-empty-folder                             |
    #         | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |


    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @run-500
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        And the user has waited for folder "<folderName>" to be synced
        When the user deletes the folder "<folderName>"
        And the user clicks on the activity tab
        And the user selects "Sync Protocol" tab in the activity
        Then the file "<folderName>" should be deleted
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |