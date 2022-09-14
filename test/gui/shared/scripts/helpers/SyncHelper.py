import re


# File syncing in client has the following status
SYNC_STATUS = {
    'SYNC': 'STATUS:SYNC',  # sync in process
    'OK': 'STATUS:OK',  # sync completed
    'ERROR': 'STATUS:ERROR',  # file sync has error
    'IGNORE': 'STATUS:IGNORE',  # file is igored
    'NOP': 'STATUS:NOP',  # file yet to be synced
    'REGISTER': 'REGISTER_PATH',
    'UNREGISTER': 'UNREGISTER_PATH',
    'UPDATE': 'UPDATE_VIEW',
}

# default sync patterns for the initial sync (after adding account)
SYNC_PATTERNS = {
    'root_sync': [
        # pattern 1
        # empty
        {
            'length': 7,
            'pattern': {
                SYNC_STATUS['OK']: [0, 2, 4, 5],
                SYNC_STATUS['REGISTER']: [1],
                SYNC_STATUS['UPDATE']: [3, 6],
            },
        },
        # pattern 2
        # only hidden files
        {
            'length': 8,
            'pattern': {
                SYNC_STATUS['OK']: [0, 2, 4, 5, 6],
                SYNC_STATUS['REGISTER']: [1],
                SYNC_STATUS['UPDATE']: [3, 7],
            },
        },
        # pattern 3
        # single folder
        # single file
        # multiple folders
        # folders and a file
        {
            'length': 10,
            'pattern': {
                SYNC_STATUS['OK']: [0, 2, 6, 7, 8],
                SYNC_STATUS['REGISTER']: [1],
                SYNC_STATUS['SYNC']: [4, 5],
                SYNC_STATUS['UPDATE']: [3, 9],
            },
        },
        # pattern 4
        # multiple files
        # a file and a folder
        # files and a folder
        {
            'length': 15,
            'pattern': {
                SYNC_STATUS['OK']: [0, 2, 6, 7, 8, 10, 12, 13],
                SYNC_STATUS['REGISTER']: [1],
                SYNC_STATUS['SYNC']: [4, 5],
                SYNC_STATUS['UPDATE']: [3, 9, 11, 14],
            },
        },
    ],
    'delete_sync': {
        'length': 5,
        'pattern': {
            SYNC_STATUS['UNREGISTER']: [0],
            SYNC_STATUS['SYNC']: [1, 2],
            SYNC_STATUS['REGISTER']: [3],
            SYNC_STATUS['OK']: [4],
        },
    },
    'edit_sync': {
        'length': 3,
        'pattern': {
            SYNC_STATUS['SYNC']: [0, 1],
            SYNC_STATUS['OK']: [2],
        },
    },
}


# generate sync pattern from pattern meta data
#
# returns List
# e.g: ['STATUS:OK', 'REGISTER_PATH', 'STATUS:OK', 'UPDATE_VIEW']
def generateSyncPattern(pattern_meta):
    pattern = [None] * pattern_meta['length']
    for status in pattern_meta['pattern']:
        for idx in pattern_meta['pattern'][status]:
            pattern[idx] = status
    return pattern


def getDefaultSyncPatterns():
    patterns = []
    for pattern_meta in SYNC_PATTERNS['root_sync']:
        patterns.append(generateSyncPattern(pattern_meta))

    return patterns


# generate sync pattern from the socket messages
#
# returns List
# e.g: ['STATUS:OK', 'REGISTER_PATH', 'STATUS:OK', 'UPDATE_VIEW']
def generateSyncPatternFromMessage(messages):
    pattern = []
    if not messages:
        return pattern

    sync_messages = getSyncMessages(messages)
    for message in sync_messages:
        # E.g; from "STATUS:OK:/tmp/client-bdd/Alice/"
        # excludes ":/tmp/client-bdd/Alice/"
        # adds only "STATUS:OK" to the pattern list
        match = re.search(":/.*", message)
        if match:
            (end, _) = match.span()
            pattern.append(message[:end])
    return pattern


# strip out the messages that are not related to sync
def getSyncMessages(messages):
    start_idx = 0
    if 'GET_STRINGS:END' in messages:
        start_idx = messages.index('GET_STRINGS:END') + 1
    return messages[start_idx:]
