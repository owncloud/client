# -*- coding: utf-8 -*-
import os
from os import listdir, rename
from os.path import isfile, join, isdir
import re
import builtins
import shutil

from pageObjects.SyncConnectionWizard import SyncConnectionWizard
from pageObjects.SyncConnection import SyncConnection
from pageObjects.Toolbar import Toolbar
from pageObjects.Activity import Activity
from pageObjects.AccountSetting import AccountSetting

from helpers.SetupClientHelper import *
from helpers.FilesHelper import buildConflictedRegex, sanitizePath
from helpers.SocketHelper import (
    getSocketConnection,
    readSocketMessages,
    clearSocketMessages,
    readAndUpdateSocketMessages,
)
from helpers.SyncHelper import (
    SYNC_STATUS,
    getInitialSyncPatterns,
    getSyncedPattern,
    generateSyncPatternFromMessages,
    filterMessagesForItem,
)


def listenSyncStatusForItem(item, type='FOLDER'):
    type = type.upper()
    if type != 'FILE' and type != 'FOLDER':
        raise Exception("type must be 'FILE' or 'FOLDER'")
    socketConnect = getSocketConnection()
    socketConnect.sendCommand("RETRIEVE_" + type + "_STATUS:" + item + "\n")


def getCurrentSyncStatus(resource, resourceType):
    listenSyncStatusForItem(resource, resourceType)
    messages = filterMessagesForItem(readSocketMessages(), resource)
    # return the last message from the list
    return messages[-1]


def waitForFileOrFolderToSync(
    context, resource='', resourceType='FOLDER', patterns=None
):
    resource = join(context.userData['currentUserSyncPath'], resource).rstrip('/')
    listenSyncStatusForItem(resource, resourceType)

    timeout = context.userData['maxSyncTimeout'] * 1000

    if patterns is None:
        patterns = getSyncedPattern()

    synced = waitFor(
        lambda: hasSyncPattern(patterns, resource),
        timeout,
    )
    clearSocketMessages(resource)
    if not synced:
        # if the sync pattern doesn't match then check the last sync status
        # and pass the step if the last sync status is STATUS:OK
        status = getCurrentSyncStatus(resource, resourceType)
        if status.startswith(SYNC_STATUS['OK']):
            test.log(
                "[WARN] Failed to match sync pattern for resource: "
                + resource
                + "\nBut its last status is "
                + "'"
                + SYNC_STATUS['OK']
                + "'"
                + ". So passing the step."
            )
            return
        else:
            raise Exception(
                "Timeout while waiting for sync to complete for "
                + str(timeout)
                + " milliseconds"
            )


def waitForInitialSyncToComplete(context):
    waitForFileOrFolderToSync(
        context,
        context.userData['currentUserSyncPath'],
        'FOLDER',
        getInitialSyncPatterns(),
    )


def hasSyncPattern(patterns, resource=None):
    if isinstance(patterns[0], str):
        patterns = [patterns]
    messages = readAndUpdateSocketMessages()
    if resource:
        messages = filterMessagesForItem(messages, resource)
    for pattern in patterns:
        pattern_len = len(pattern)
        for idx, _ in enumerate(messages):
            actual_pattern = generateSyncPatternFromMessages(
                messages[idx : idx + pattern_len]
            )
            if len(actual_pattern) < pattern_len:
                break
            if pattern_len == len(actual_pattern) and pattern == actual_pattern:
                return True
    # 100 milliseconds polling interval
    snooze(0.1)
    return False


# Using socket API to check file sync status
def hasSyncStatus(itemName, status):
    sync_messages = readAndUpdateSocketMessages()
    sync_messages = filterMessagesForItem(sync_messages, itemName)
    for line in sync_messages:
        if line.startswith(status) and line.rstrip('/').endswith(itemName.rstrip('/')):
            return True
    return False


# useful for checking sync status such as 'error', 'ignore'
# but not quite so reliable for checking 'ok' sync status
def waitForFileOrFolderToHaveSyncStatus(
    context, resource, resourceType, status=SYNC_STATUS['OK'], timeout=None
):
    resource = sanitizePath(join(context.userData['currentUserSyncPath'], resource))

    listenSyncStatusForItem(resource, resourceType)

    if not timeout:
        timeout = context.userData['maxSyncTimeout'] * 1000

    result = waitFor(
        lambda: hasSyncStatus(resource, status),
        timeout,
    )

    if not result:
        if status == SYNC_STATUS['ERROR']:
            expected = "have sync error"
        elif status == SYNC_STATUS['IGNORE']:
            expected = "be sync ignored"
        else:
            expected = "be synced"
        raise Exception(
            "Expected "
            + resourceType
            + " '"
            + resource
            + "' to "
            + expected
            + ", but not."
        )


def waitForFileOrFolderToHaveSyncError(context, resource, resourceType):
    waitForFileOrFolderToHaveSyncStatus(
        context, resource, resourceType, SYNC_STATUS['ERROR']
    )


def folderExists(folderPath, timeout=1000):
    return waitFor(
        lambda: isdir(sanitizePath(folderPath)),
        timeout,
    )


def fileExists(filePath, timeout=1000):
    return waitFor(
        lambda: isfile(sanitizePath(filePath)),
        timeout,
    )


@When('the user waits for the files to sync')
def step(context):
    waitForFileOrFolderToSync(context)


@When(r'the user waits for (file|folder) "([^"]*)" to be synced', regexp=True)
def step(context, type, resource):
    waitForFileOrFolderToSync(context, resource, type)


@When(r'the user waits for (file|folder) "([^"]*)" to have sync error', regexp=True)
def step(context, type, resource):
    waitForFileOrFolderToHaveSyncError(context, resource, type)


@When(
    r'user "([^"]*)" waits for (file|folder) "([^"]*)" to have sync error', regexp=True
)
def step(context, username, type, resource):
    resource = join(getUserSyncPath(context, username), resource)
    waitForFileOrFolderToHaveSyncError(context, resource, type)


@When(
    'user "|any|" creates a file "|any|" with the following content inside the sync folder'
)
def step(context, username, filename):
    fileContent = "\n".join(context.multiLineText)
    syncPath = getUserSyncPath(context, username)
    waitAndWriteFile(context, join(syncPath, filename), fileContent)


@When('user "|any|" creates a folder "|any|" inside the sync folder')
def step(context, username, foldername):
    createFolder(context, foldername, username)


@Given('user "|any|" has created a folder "|any|" inside the sync folder')
def step(context, username, foldername):
    createFolder(context, foldername, username)


# To create folders in a temporary directory, we set isTempFolder True
# And if isTempFolder is True, the createFolder function create folders in tempFolderPath
def createFolder(context, foldername, username=None, isTempFolder=False):
    syncPath = None
    if username and not isTempFolder:
        syncPath = getUserSyncPath(context, username)
    elif isTempFolder:
        syncPath = context.userData['tempFolderPath']
    else:
        syncPath = context.userData['currentUserSyncPath']
    path = join(syncPath, foldername)
    os.makedirs(path)


def renameFileFolder(context, source, destination):
    source = join(context.userData['currentUserSyncPath'], source)
    destination = join(context.userData['currentUserSyncPath'], destination)
    rename(source, destination)


@When('user "|any|" creates a file "|any|" with size "|any|" inside the sync folder')
def step(context, username, filename, filesize):
    createFileWithSize(context, filename, filesize)


def createFileWithSize(context, filename, filesize, isTempFolder=False):
    if isTempFolder:
        path = context.userData['tempFolderPath']
    else:
        path = context.userData['currentUserSyncPath']
    file = join(path, filename)
    cmd = "truncate -s {filesize} {file}".format(filesize=filesize, file=file)
    os.system(cmd)


@When('the user copies the folder "|any|" to "|any|"')
def step(context, sourceFolder, destinationFolder):
    source_dir = join(context.userData['currentUserSyncPath'], sourceFolder)
    destination_dir = join(context.userData['currentUserSyncPath'], destinationFolder)
    shutil.copytree(source_dir, destination_dir)


@When(r'the user renames a (file|folder) "([^"]*)" to "([^"]*)"', regexp=True)
def step(context, type, source, destination):
    renameFileFolder(context, source, destination)


@Then('the file "|any|" should exist on the file system with the following content')
def step(context, filePath):
    expected = "\n".join(context.multiLineText)
    filePath = context.userData['currentUserSyncPath'] + filePath
    f = open(filePath, 'r')
    contents = f.read()
    test.compare(
        expected,
        contents,
        "file expected to exist with content "
        + expected
        + " but does not have the expected content",
    )


@Then(r'^the (file|folder) "([^"]*)" should exist on the file system$', regexp=True)
def step(context, resourceType, resource):
    resourcePath = join(context.userData['currentUserSyncPath'], resource)
    resourceExists = False
    if resourceType == 'file':
        resourceExists = fileExists(
            resourcePath, context.userData['maxSyncTimeout'] * 1000
        )
    elif resourceType == 'folder':
        resourceExists = folderExists(
            resourcePath, context.userData['maxSyncTimeout'] * 1000
        )
    else:
        raise Exception("Unsupported resource type '" + resourceType + "'")

    test.compare(
        True,
        resourceExists,
        "Assert " + resourceType + " '" + resource + "' exists on the system",
    )


@Then(r'^the (file|folder) "([^"]*)" should not exist on the file system$', regexp=True)
def step(context, resourceType, resource):
    resourcePath = join(context.userData['currentUserSyncPath'], resource)
    resourceExists = False
    if resourceType == 'file':
        resourceExists = fileExists(resourcePath, 1000)
    elif resourceType == 'folder':
        resourceExists = folderExists(resourcePath, 1000)
    else:
        raise Exception("Unsupported resource type '" + resourceType + "'")

    test.compare(
        False,
        resourceExists,
        "Assert " + resourceType + " '" + resource + "' doesn't exist on the system",
    )


@Given('the user has paused the file sync')
def step(context):
    SyncConnection.pauseSync(context)


@Given('the user has changed the content of local file "|any|" to:')
def step(context, filename):
    fileContent = "\n".join(context.multiLineText)
    waitAndWriteFile(
        context, join(context.userData['currentUserSyncPath'], filename), fileContent
    )


@When('the user resumes the file sync on the client')
def step(context):
    SyncConnection.resumeSync(context)


@Then(
    'a conflict file for "|any|" should exist on the file system with the following content'
)
def step(context, filename):
    expected = "\n".join(context.multiLineText)

    onlyfiles = [
        f
        for f in listdir(context.userData['currentUserSyncPath'])
        if isfile(join(context.userData['currentUserSyncPath'], f))
    ]
    found = False
    pattern = re.compile(buildConflictedRegex(filename))
    for file in onlyfiles:
        if pattern.match(file):
            f = open(context.userData['currentUserSyncPath'] + file, 'r')
            contents = f.read()
            if contents == expected:
                found = True
                break

    if not found:
        raise Exception("Conflict file not found with given name")


@When('the user clicks on the activity tab')
def step(context):
    Toolbar.openActivity()


@Then('the table of conflict warnings should include file "|any|"')
def step(context, filename):
    Activity.checkFileExist(filename)


@Then('the file "|any|" should be blacklisted')
def step(context, filename):
    test.compare(
        True,
        Activity.checkBlackListedResourceExist(context, filename),
        "File is blacklisted",
    )


@When('the user selects "|any|" tab in the activity')
def step(context, tabName):
    Activity.clickTab(tabName)


@Then("the following tabs in the toolbar should match the default baseline")
def step(context):
    for tabName in context.table:
        test.vp(tabName[0])


# performing actions immediately after completing the sync from the server does not work
# The test should wait for a while before performing the action
# issue: https://github.com/owncloud/client/issues/8832
def waitForClientToBeReady(context):
    global waitedAfterSync
    if not waitedAfterSync:
        snooze(context.userData['minSyncTimeout'])
        waitedAfterSync = True


def writeFile(resource, content):
    f = open(resource, "w")
    f.write(content)
    f.close()


def waitAndWriteFile(context, path, content):
    waitForClientToBeReady(context)
    writeFile(path, content)


def waitAndTryToWriteFile(context, resource, content):
    waitForClientToBeReady(context)
    try:
        writeFile(resource, content)
    except:
        pass


@When('the user overwrites the file "|any|" with content "|any|"')
def step(context, resource, content):
    print("starting file overwrite")
    resource = join(context.userData['currentUserSyncPath'], resource)
    waitAndWriteFile(context, resource, content)
    print("file has been overwritten")


@When('the user tries to overwrite the file "|any|" with content "|any|"')
def step(context, resource, content):
    resource = context.userData['currentUserSyncPath'] + resource
    waitAndTryToWriteFile(context, resource, content)


@When('user "|any|" tries to overwrite the file "|any|" with content "|any|"')
def step(context, user, resource, content):
    resource = getResourcePath(context, resource, user)
    waitAndTryToWriteFile(context, resource, content)


@When("the user enables virtual file support")
def step(context):
    SyncConnection.enableVFS(context)


@Then('the "|any|" button should be available')
def step(context, item):
    SyncConnection.openMenu(context)
    SyncConnection.hasMenuItem(item)


@Given("the user has enabled virtual file support")
def step(context):
    SyncConnection.enableVFS(context)


@When("the user disables virtual file support")
def step(context):
    SyncConnection.disableVFS(context)


@When(r'the user deletes the (file|folder) "([^"]*)"', regexp=True)
def step(context, itemType, resource):
    waitForClientToBeReady(context)

    resourcePath = sanitizePath(context.userData['currentUserSyncPath'] + resource)
    if itemType == 'file':
        os.remove(resourcePath)
    elif itemType == 'folder':
        shutil.rmtree(resourcePath)
    else:
        raise Exception("No such item type for resource")

    isSyncFolderEmpty = True
    for item in listdir(context.userData['currentUserSyncPath']):
        # do not count the hidden files as they are ignored by the client
        if not item.startswith("."):
            isSyncFolderEmpty = False
            break

    # if the sync folder is empty after deleting file,
    # a dialog will popup asking to confirm "Remove all files"
    if isSyncFolderEmpty:
        try:
            AccountSetting.confirmRemoveAllFiles()
        except:
            pass


@When('the user selects the following folders to sync:')
def step(context):
    SyncConnectionWizard.selectFoldersToSync(context)
    SyncConnectionWizard.addSyncConnection()


@When('the user sorts the folder list by "|any|"')
def step(context, headerText):
    headerText = headerText.capitalize()
    if headerText in ["Size", "Name"]:
        SyncConnectionWizard.sortBy(headerText)
    else:
        raise Exception("Sorting by '" + headerText + "' is not supported.")


@Then('the sync all checkbox should be checked')
def step(context):
    test.compare(
        SyncConnectionWizard.isRootFolderChecked(), True, "Sync all checkbox is checked"
    )


@Then("the folders should be in the following order:")
def step(context):
    rowIndex = 0
    for row in context.table[1:]:
        expectedFolder = row[0]
        actualFolder = SyncConnectionWizard.getItemNameFromRow(rowIndex)
        test.compare(actualFolder, expectedFolder)

        rowIndex += 1


@Then('VFS enabled baseline image should match the default screenshot')
def step(context):
    if context.userData['ocis']:
        test.vp("VP_VFS_enabled_oCIS")
    else:
        test.vp("VP_VFS_enabled")


@Then('VFS enabled baseline image should not match the default screenshot')
def step(context):
    if context.userData['ocis']:
        test.xvp("VP_VFS_enabled_oCIS")
    else:
        test.xvp("VP_VFS_enabled")


@When('user "|any|" creates the following files inside the sync folder:')
def step(context, username):
    syncPath = getUserSyncPath(context, username)

    waitForClientToBeReady(context)

    for row in context.table[1:]:
        filename = syncPath + row[0]
        writeFile(join(syncPath, filename), '')


@Given(
    'the user has created a folder "|any|" with "|any|" files each of size "|any|" bytes in temp folder'
)
def step(context, foldername, filenumber, filesize):
    createFolder(context, foldername, isTempFolder=True)
    filesize = builtins.int(filesize)
    for i in range(0, builtins.int(filenumber)):
        filename = f"file{i}.txt"
        createFileWithSize(context, join(foldername, filename), filesize, True)


@When('user "|any|" moves folder "|any|" from the temp folder into the sync folder')
def step(context, username, foldername):
    source_dir = join(context.userData['tempFolderPath'], foldername)
    destination_dir = getUserSyncPath(context, username)
    shutil.move(source_dir, destination_dir)


@When('the user sets the sync path in sync connection wizard')
def step(context):
    SyncConnectionWizard.setSyncPathInSyncConnectionWizard(context)


@When('the user selects "|any|" as a remote destination folder')
def step(context, folderName):
    SyncConnectionWizard.selectRemoteDestinationFolder(folderName)
