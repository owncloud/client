Feature: deleting files and folders

  	As a user
  	I want to delete files and folders
  	So that I can keep my filing system clean and tidy


	Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
    
    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
    
    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
    
    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
    
    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
    
    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |

    @issue-9439 @debug
    Scenario Outline: Delete a folder
        Given user "Alice" has created folder "<folderName>" on the server
        And user "Alice" has set up a client with default settings
        When the user waits for the files to sync
        And the user deletes the folder "<folderName>"
        And the user waits for the files to sync
        Then as "Alice" file "<folderName>" should not exist on the server
        Examples:
            | folderName                                      |
            | simple-empty-folder                             |
            | simple-folder-with-name-more-than-20-characters |
