from urllib.parse import quote
import xml.etree.ElementTree as ET
import json
import os

import helpers.api.http_helper as request
from helpers.api.utils import url_join
from helpers.ConfigHelper import get_config
from helpers.FilesHelper import get_file_for_upload
from helpers.UserHelper import get_username_for_user
from helpers.api.provisioning import created_users

tokens = {}
personal_sync_folder_id = {}


def get_headers(user, extra_headers=None):
    token = generate_token(user)
    auth_headers = {
        'x-accellion-version': '28',
        'x-xsrf-token': token['x_xsrf_token'],
        'cookie': f'ACC-XSRF-TOKEN={token["acc_xsrf_token"]}; web_token={token["web_token"]}',
    }
    if extra_headers:
        auth_headers.update(extra_headers)
    return auth_headers


def get_webdav_url():
    return url_join(get_config('localBackendUrl'), 'remote.php/dav/files')


def get_resource_path(user, resource):
    resource = resource.strip('/').replace('\\', '/')
    encoded_resource_path = [quote(path, safe='') for path in resource.split('/')]
    encoded_resource_path = '/'.join(encoded_resource_path)
    url = url_join(get_webdav_url(), user, encoded_resource_path)
    return url


def resource_exists(user, resource):
    response = request.propfind(get_resource_path(user, resource), user=user)
    if response.status_code == 207:
        return True
    if response.status_code == 404:
        return False
    raise AssertionError(f'Server returned status code: {response.status_code}')


def get_file_content(user, resource):
    response = request.get(get_resource_path(user, resource), user=user)
    return response.text


def get_folder_items_count(user, folder_name):
    folder_name = folder_name.strip('/')
    path = get_resource_path(user, folder_name)
    xml_response = request.propfind(path, user=user)
    total_items = 0
    root_element = ET.fromstring(xml_response.content)
    for response_element in root_element:
        for href_element in response_element:
            # The first item is folder itself so excluding it
            if href_element.tag == '{DAV:}href' and not href_element.text.endswith(
                f'{user}/{folder_name}/'
            ):
                total_items += 1
    return str(total_items)


def create_folder(user, folder_name):
    if get_config('predefined_users'):
        user = get_username_for_user(user)
        url = url_join(
            get_config('localBackendUrl'),
            f'rest/folders/{get_personal_sync_folder_id(user)}/folders',
        )
        headers = get_headers(user, {'content-type': 'application/json'})
        body = json.dumps(
            {
                'description': '',
                'fileLifetime': '0',
                'name': folder_name,
                'syncable': '1',
            }
        )
        response = request.post(url, body, headers)
    else:
        url = get_resource_path(user, folder_name)
        response = request.mkcol(url, user=user)
    assert (
        response.status_code == 201
    ), f'Could not create the folder: {folder_name} for user {user}'


def create_file(user, file_name, contents):
    url = get_resource_path(user, file_name)
    response = request.put(url, body=contents, user=user)
    assert response.status_code in [
        201,
        204,
    ], f"Could not create file '{file_name}' for user {user}"


def upload_file(user, file_name, destination=None):
    if get_config('predefined_users'):
        file_path, file_size = get_local_file_info(file_name)

        # initiate upload
        upload_uri = initiate_upload(user, file_name, file_size)

        # finalize upload
        finalize_upload(user, file_name, file_path, file_size, upload_uri)
    else:
        file_path = get_file_for_upload(file_name)
        with open(file_path, 'rb') as file:
            contents = file.read()
        create_file(user, destination, contents)


def get_local_file_info(file_name):
    script_path = os.path.dirname(os.path.realpath(__file__))
    file_path = os.path.abspath(
        os.path.join(script_path, '..', '..', '..', 'files-for-upload', file_name)
    )
    file_size = os.path.getsize(file_path)
    return file_path, file_size


def initiate_upload(user, file_name, file_size):
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/folders/{get_personal_sync_folder_id(user)}/actions/initiateUpload',
    )
    headers = get_headers(user, {'content-type': 'application/json'})
    json_body = json.dumps(
        {'filename': file_name, 'totalSize': file_size, 'totalChunks': 1}
    )

    response = request.post(url, body=json_body, headers=headers)
    assert (
        response.status_code == 201
    ), f'Failed to initiate upload for file {file_name}.'

    data = json.loads(response.text)
    return data['uri']


def finalize_upload(user, file_name, file_path, file_size, upload_uri):
    url = url_join(get_config('localBackendUrl'), upload_uri)
    headers = get_headers(user)
    with open(file_path, 'rb') as f:
        files = {'content': (file_name, f, 'application/octet-stream')}
        body = {
            'compressionMode': 'NORMAL',
            'compressionSize': file_size,
            'originalSize': file_size,
            'index': 0,
        }
        response = request.post(url, body=body, headers=headers, files=files)
        assert response.status_code == 201, f'Failed to upload file {file_name}.'


def delete_resource(user, resource):
    url = get_resource_path(user, resource)
    response = request.delete(url, user=user)
    assert response.status_code == 204, f"Could not delete folder '{resource}'"


def generate_token(user):
    print(user)
    if user in tokens:
        return tokens[user]
    url = url_join(get_config('localBackendUrl'), 'rest/users/actions/login')
    headers = {'content-type': 'application/json', 'x-accellion-version': '28'}
    body = json.dumps({'username': user, 'password': 'at*f1WM07IGej@fT'})
    response = request.send_request(url, 'POST', body, headers)
    assert response.status_code == 200, 'Failed to get the token'

    acc_xsrf_token = None
    web_token = None
    x_xsrf_token = None
    for cookie in response.cookies:
        if cookie.name == 'ACC-XSRF-TOKEN':
            acc_xsrf_token = cookie.value
            x_xsrf_token = cookie.value
        elif cookie.name == 'web_token':
            web_token = cookie.value
    tokens[user] = {
        'acc_xsrf_token': acc_xsrf_token,
        'web_token': web_token,
        'x_xsrf_token': x_xsrf_token,
    }
    return tokens[user]


def get_personal_sync_folder_id(user):
    if user in personal_sync_folder_id:
        return personal_sync_folder_id[user]

    url = url_join(get_config('localBackendUrl'), 'rest/folders/top')
    headers = get_headers(user)
    response = request.get(url, headers)
    assert (
        response.status_code == 200
    ), f'Failed to get the personal sync folder id for user {user}'
    data = json.loads(response.text)

    for folder in data['data']:
        if folder['name'] == get_config('personal_sync_folder'):
            personal_sync_folder_id[user] = folder['id']
            return personal_sync_folder_id[user]
    raise ValueError(f'Cound not find personal sync folder id for user {user}')


# return resources inside sync folder
# status=False returns resources that are not deleted
# status=True returns resources that are deleted and are stored in deleted content
def get_resources_inside_sync_folder(user, status=False):
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/folders/{get_personal_sync_folder_id(user)}/children?deleted={status}',
    )
    headers = get_headers(user)
    response = request.get(url, headers)
    assert response.status_code == 200, 'Failed to get the resources inside sync folder'

    data = json.loads(response.text)
    return data['data']


def get_resource_ids(user, status=False):
    resources = get_resources_inside_sync_folder(user, status)
    folder_ids = []
    file_ids = []
    for resource in resources:
        if resource['type'] == 'd':
            folder_ids.append(resource['id'])
        if resource['type'] == 'f':
            file_ids.append(resource['id'])
    return folder_ids, file_ids


def delete_all_resources():
    for user in created_users.values():
        folder_ids, file_ids = get_resource_ids(user['username'])
        headers = get_headers(user['username'])
        # Delete all folders
        if len(folder_ids) > 0:
            url = url_join(
                get_config('localBackendUrl'),
                f'rest/folders?partialSuccess=true&id:in={",".join(folder_ids)}',
            )
            response = request.delete(url, headers)
            assert response.status_code == 204, 'Failed to delete all folders.'

        # Delete all files
        if len(file_ids) > 0:
            url = url_join(
                get_config('localBackendUrl'),
                f'rest/files?partialSuccess=true&id:in={",".join(file_ids)}',
            )
            response = request.delete(url, headers)
            assert response.status_code == 204, 'Failed to delete all files.'


def permanently_delete_all_resources():
    for user in created_users.values():
        folder_ids, file_ids = get_resource_ids(user['username'], True)
        headers = get_headers(user['username'])
        # Permanently delete all folders
        if len(folder_ids) > 0:
            url = url_join(
                get_config('localBackendUrl'),
                f'rest/folders/actions/permanent?partialSuccess=true&id:in={",".join(folder_ids)}',
            )
            response = request.delete(url, headers)
            assert (
                response.status_code == 204
            ), 'Failed to permanently delete all folders.'

        # Permanently delete all files
        if len(file_ids) > 0:
            url = url_join(
                get_config('localBackendUrl'),
                f'rest/files/actions/permanent?partialSuccess=true&id:in={",".join(file_ids)}',
            )
            response = request.delete(url, headers)
            assert (
                response.status_code == 204
            ), 'Failed to permanently delete all files.'
