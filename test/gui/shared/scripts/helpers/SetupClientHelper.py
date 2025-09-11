import os
import subprocess
from urllib.parse import urlparse
from os import makedirs
from os.path import exists, join
import test
import psutil
import squish

from helpers.ConfigHelper import get_config, set_config, is_windows, is_owncloud_client
from helpers.ReportHelper import is_video_enabled
from helpers.UserHelper import get_username_for_user
from helpers.api.provisioning import created_users


def substitute_inline_codes(value):
    value = value.replace('%local_server%', get_config('localBackendUrl'))
    value = value.replace('%secure_local_server%', get_config('secureLocalBackendUrl'))
    value = value.replace('%client_root_sync_path%', get_config('clientRootSyncPath'))
    value = value.replace('%current_user_sync_path%', get_config('currentUserSyncPath'))
    value = value.replace(
        '%local_server_hostname%', urlparse(get_config('localBackendUrl')).netloc
    )
    value = value.replace('%home%', get_config('home_dir'))

    return value


def get_client_details(details):
    client_details = {
        'server': '',
        'user': '',
        'password': '',
        'sync_folder': '',
        'oauth': False,
    }
    for row in details:
        row[1] = substitute_inline_codes(row[1])
        if row[0] == 'server':
            client_details.update({'server': row[1]})
        elif row[0] == 'user':
            client_details.update({'user': get_username_for_user(row[1])})
        elif row[0] == 'password':
            if not is_owncloud_client():
                for created_user in created_users.values():
                    if created_user['username'] == client_details['user']:
                        client_details.update({'password': created_user['password']})
                        break
            else:
                client_details.update({'password': row[1]})
        elif row[0] == 'sync_folder':
            client_details.update({'sync_folder': row[1]})
    return client_details


def create_user_sync_path(username):
    # '' at the end adds '/' to the path
    user_sync_path = join(get_config('clientRootSyncPath'), username, '')

    if not exists(user_sync_path):
        makedirs(user_sync_path)

    set_current_user_sync_path(user_sync_path)
    return user_sync_path.replace('\\', '/')


def create_space_path(space='Personal'):
    space_path = join(get_config('currentUserSyncPath'), space, '')
    if not exists(space_path):
        makedirs(space_path)
    return space_path.replace('\\', '/')


def set_current_user_sync_path(sync_path):
    set_config('currentUserSyncPath', sync_path)


def get_resource_path(resource='', user='', space=''):
    sync_path = get_config('currentUserSyncPath')
    if user:
        sync_path = user
    if get_config('ocis'):
        space = space or get_config('syncConnectionName')
        sync_path = join(sync_path, space)
    sync_path = join(get_config('clientRootSyncPath'), sync_path)
    resource = resource.replace(sync_path, '').strip('/').strip('\\')
    if is_windows():
        resource = resource.replace('/', '\\')
    return join(
        sync_path,
        resource,
    )


def get_temp_resource_path(resource_name):
    return join(get_config('tempFolderPath'), resource_name)


def get_current_user_sync_path():
    return get_config('currentUserSyncPath')


def start_client():
    check_keyring()

    squish.startApplication(
        f'{get_config("client_name").lower()} -s'
        + f' --logfile {get_config("clientLogFile")}'
        + ' --logdebug'
        + ' --logflush'
    )
    if is_video_enabled():
        test.startVideoCapture()
    else:
        test.log(
            f'Video recordings reached the maximum limit of {get_config("video_record_limit")}.'
            + 'Skipping video recording...'
        )


def is_app_killed(pid):
    try:
        psutil.Process(pid)
        return False
    except psutil.NoSuchProcess:
        return True


def wait_until_app_killed(pid=0):
    timeout = 5 * 1000
    killed = squish.waitFor(
        lambda: is_app_killed(pid),
        timeout,
    )
    if not killed:
        test.log(f'Application was not terminated within {timeout} milliseconds')


# sometimes the keyring is locked during the test execution, and we need to unlock it
def check_keyring():
    if is_windows():
        return

    if is_keyring_locked():
        test.log('[INFO] Keyring is locked or service is down. Unlocking...')
        wait_until_keyring_unlocked()


def unlock_keyring() -> bool | None:
    if is_windows():
        return None

    password = os.getenv('VNC_PW')
    command = f'echo -n "{password}" | gnome-keyring-daemon -r --unlock'
    stdout, stderr, returncode = run_sys_command(command, True)

    output = ''
    if stdout:
        output = stdout.decode('utf-8')
    if stderr:
        output = stderr.decode('utf-8')
    test.log(f'[INFO] Keyring unlock output: {output}')
    # wait for keyring to unlock
    squish.snooze(1)

    if returncode:
        return False

    return not is_keyring_locked()


def wait_until_keyring_unlocked():
    if is_windows():
        return

    timeout = 10
    unlocked = squish.waitFor(
        lambda: unlock_keyring(),  # pylint: disable=unnecessary-lambda
        timeout * 1000,
    )
    if not unlocked:
        test.fail(f'Timeout. Keyring was not unlocked within {timeout} seconds')


def is_keyring_locked() -> bool | None:
    if is_windows():
        return None

    stdout, stderr, _ = run_sys_command(
        [
            'busctl',
            '--user',
            'get-property',
            'org.freedesktop.secrets',
            '/org/freedesktop/secrets/collection/login',
            'org.freedesktop.Secret.Collection',
            'Locked',
        ]
    )
    output = ''
    if stdout:
        output = stdout.decode('utf-8')
    if stderr:
        output = stderr.decode('utf-8')
    test.log(f'[INFO] Keyring locked status: {output}')
    return not output.strip().endswith('false')


def run_sys_command(command=None, shell=False):
    cmd = subprocess.run(
        command,
        shell=shell,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return cmd.stdout, cmd.stderr, cmd.returncode
