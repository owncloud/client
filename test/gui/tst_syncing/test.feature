@tests
Feature: Syncing files

    As a user
    I want to be able to sync my local folders to to my owncloud server
    so that I dont have to upload and download files manually

    Background:
        Given user "Alice" has been created on the server with default attributes and without skeleton files


    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |


    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |


    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted1
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |

    Scenario Outline: File with long name (233 characters) is blacklisted
        Given user "Alice" has set up a client with default settings
        When user "Alice" creates a file "<filename>" with the following content inside the sync folder
            """
            test contents
            """
        And the user clicks on the activity tab
        And the user selects "Not Synced" tab in the activity
        Then the file "<filename>" should be blacklisted
        Examples:
            | filename                                                                                                                                                                                                                                  |
            | thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFileNameToCheckThatItWorks-thisIsAVeryLongFile.txt |
