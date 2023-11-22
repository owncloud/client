from urllib.parse import urlparse
import squish
import uuid
from os import makedirs
from os.path import exists, join
from helpers.SpaceHelper import get_space_id
from helpers.ConfigHelper import get_config, set_config
from helpers.SyncHelper import listenSyncStatusForItem
from helpers.UserHelper import getDisplaynameForUser


def substituteInLineCodes(value):
    value = value.replace('%local_server%', get_config('localBackendUrl'))
    value = value.replace('%secure_local_server%', get_config('secureLocalBackendUrl'))
    value = value.replace('%client_root_sync_path%', get_config('clientRootSyncPath'))
    value = value.replace('%current_user_sync_path%', get_config('currentUserSyncPath'))
    value = value.replace(
        '%local_server_hostname%', urlparse(get_config('localBackendUrl')).netloc
    )

    return value


def getClientDetails(context):
    clientDetails = {'server': '', 'user': '', 'password': '', 'sync_folder': ''}
    for row in context.table[0:]:
        row[1] = substituteInLineCodes(row[1])
        if row[0] == 'server':
            clientDetails.update({'server': row[1]})
        elif row[0] == 'user':
            clientDetails.update({'user': row[1]})
        elif row[0] == 'password':
            clientDetails.update({'password': row[1]})
        elif row[0] == 'sync_folder':
            clientDetails.update({'sync_folder': row[1]})
    return clientDetails


def createUserSyncPath(username):
    # '' at the end adds '/' to the path
    userSyncPath = join(get_config('clientRootSyncPath'), username, '')

    if not exists(userSyncPath):
        makedirs(userSyncPath)

    setCurrentUserSyncPath(userSyncPath)
    return userSyncPath


def createSpacePath(space='Personal'):
    spacePath = join(get_config('currentUserSyncPath'), space, '')
    if not exists(spacePath):
        makedirs(spacePath)
    return spacePath


def setCurrentUserSyncPath(syncPath):
    set_config('currentUserSyncPath', syncPath)


def getResourcePath(resource='', user='', space=''):
    sync_path = get_config('currentUserSyncPath')
    if user:
        sync_path = user
    if get_config('ocis'):
        space = space or get_config('syncConnectionName')
        sync_path = join(sync_path, space)
    sync_path = join(get_config('clientRootSyncPath'), sync_path)
    resource = resource.replace(sync_path, '').strip('/')
    return join(
        sync_path,
        resource,
    )


def getTempResourcePath(resourceName):
    return join(get_config('tempFolderPath'), resourceName)


def getCurrentUserSyncPath():
    return get_config('currentUserSyncPath')


def startClient():
    squish.startApplication(
        "owncloud -s"
        + " --logfile "
        + get_config('clientLogFile')
        + " --logdebug"
        + " --logflush"
    )


def getPollingInterval():
    pollingInterval = '''
[ownCloud]
remotePollInterval={pollingInterval}
'''
    args = {'pollingInterval': 5000}
    pollingInterval = pollingInterval.format(**args)
    return pollingInterval


def generate_account_config(users, space="Personal"):
    sync_paths = []
    user_setting = ''
    for idx, username in enumerate(users):
        user_setting += '''
{user_index}/Folders/{uuid_v4}/davUrl={url}
{user_index}/Folders/{uuid_v4}/ignoreHiddenFiles=true
{user_index}/Folders/{uuid_v4}/localPath={client_sync_path}
{user_index}/Folders/{uuid_v4}/displayString={displayString}
{user_index}/Folders/{uuid_v4}/paused=false
{user_index}/Folders/{uuid_v4}/targetPath=/
{user_index}/Folders/{uuid_v4}/version=13
{user_index}/Folders/{uuid_v4}/virtualFilesMode=off
{user_index}/dav_user={davUserName}
{user_index}/display-name={displayUserName}
{user_index}/http_CredentialVersion=1
{user_index}/http_oauth={oauth}
{user_index}/http_user={davUserName}
{user_index}/url={local_server}
{user_index}/user={displayUserFirstName}
{user_index}/version=13
'''

        if not idx:
            user_setting = "[Accounts]" + user_setting

        sync_path = createUserSyncPath(username)
        dav_endpoint = join("remote.php/dav/files", username)

        server_url = get_config('localBackendUrl')
        is_ocis = get_config('ocis')
        if is_ocis:
            set_config('syncConnectionName', space)
            sync_path = createSpacePath(space)
            if space == "Personal":
                space = getDisplaynameForUser(username)
            dav_endpoint = join("dav/spaces", get_space_id(space, username))

        args = {
            'url': join(server_url, dav_endpoint, ''),
            'displayString': get_config('syncConnectionName'),
            'displayUserName': getDisplaynameForUser(username),
            'davUserName': username if is_ocis else username.lower(),
            'displayUserFirstName': getDisplaynameForUser(username).split()[0],
            'client_sync_path': sync_path,
            'local_server': server_url,
            'oauth': 'true' if is_ocis else 'false',
            'user_index': idx,
            'uuid_v4': generate_UUIDV4(),
        }
        user_setting = user_setting.format(**args)
        sync_paths.append(sync_path)
    # append extra configs
    user_setting += "version=13"
    user_setting = user_setting + getPollingInterval()

    config_file = open(get_config('clientConfigFile'), "a+", encoding="utf-8")
    config_file.write(user_setting)
    config_file.close()

    return sync_paths


def setUpClient(username, space="Personal"):
    sync_path = generate_account_config([username], space)
    startClient()
    listenSyncStatusForItem(sync_path[0])


def generate_UUIDV4():
    return str(uuid.uuid4())
