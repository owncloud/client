import os
import json

import helpers.api.http_helper as request
from helpers.api.utils import url_join
from helpers.ConfigHelper import get_config
from helpers.FilesHelper import write_file
from helpers.UserHelper import get_username_for_user
from helpers.api.provisioning import created_users
from helpers.FilesHelper import get_file_for_upload

tokens = {}
personal_sync_folder_id = {}


def get_headers(user, extra_headers=None):
    token = generate_xsrf_token(user)
    headers = {
        'x-accellion-version': '28',
        'x-xsrf-token': token['x_xsrf_token'],
        'cookie': f'ACC-XSRF-TOKEN={token["acc_xsrf_token"]}; web_token={token["web_token"]}',
    }
    if extra_headers:
        headers.update(extra_headers)
    return headers


def generate_xsrf_token(user):
    if user in tokens:
        return tokens[user]
    url = url_join(get_config('localBackendUrl'), 'rest/users/actions/login')
    headers = {'content-type': 'application/json', 'x-accellion-version': '28'}
    for created_user in created_users.values():
        if created_user['username'] == user:
            password = created_user['password']
            break
    body = json.dumps({'username': user, 'password': password})
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


def resource_exists(user, resource):
    user = get_username_for_user(user)
    resource_name = os.path.basename(resource)
    folder_path = os.path.dirname(resource)
    folder_id = get_folder_id_by_path(user, folder_path)
    return get_resource(user, resource_name, folder_id) is not None


def get_file_content(user, resource):
    user = get_username_for_user(user)
    r = get_resource(user, resource)
    url = url_join(get_config('localBackendUrl'), f'rest/files/{r["id"]}/content')
    headers = get_headers(user)
    response = request.get(url, headers=headers)
    assert response.status_code == 200, f'Could not get content of file: {resource}'
    return response.text


def create_folder(user, folder_name):
    user = get_username_for_user(user)
    folder_to_create = os.path.basename(folder_name)
    folder_path = os.path.dirname(folder_name)
    folder_id = get_folder_id_by_path(user, folder_path)
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/folders/{folder_id}/folders',
    )
    headers = get_headers(user, {'content-type': 'application/json'})
    body = json.dumps(
        {
            'description': '',
            'fileLifetime': '0',
            'name': folder_to_create,
            'syncable': '1',
        }
    )
    response = request.post(url, body, headers)
    assert (
        response.status_code == 201
    ), f'Could not create the folder: {folder_name} for user {user}'


def get_folder_id_by_path(user, folder_path=None):
    parts = folder_path.split(os.sep)
    current_folder_id = get_personal_sync_folder_id(user)
    if not folder_path:
        return current_folder_id

    for part in parts:
        if folder := get_resource(user, part, current_folder_id, 'd'):
            current_folder_id = folder['id']
    return current_folder_id


def upload_file(user, file_name, file_content=None):
    user = get_username_for_user(user)
    filename = os.path.basename(file_name)
    if file_content:
        # create file in temp folder before uploading
        file_path = f'{get_config("tempFolderPath")}/{filename}'
        write_file(file_path, file_content)
    else:
        file_path = get_file_for_upload(filename)
    file_size = os.path.getsize(file_path)
    folder_path = os.path.dirname(file_name)
    folder_id = get_folder_id_by_path(user, folder_path)

    # initiate upload
    upload_uri = initiate_upload(user, filename, file_size, folder_id)

    # finalize upload
    finalize_upload(user, filename, file_path, file_size, upload_uri)


def initiate_upload(user, file_name, file_size, folder_id):
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/folders/{folder_id}/actions/initiateUpload',
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
    user = get_username_for_user(user)
    r = get_resource(user, resource)
    if r['type'] == 'd':
        uri = 'folders'
    else:
        uri = 'files'
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/{uri}?partialSuccess=true&id:in={r["id"]}',
    )
    headers = get_headers(user)
    response = request.delete(url, headers)
    assert response.status_code == 204, f'Cound not delete resource: {resource}'


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
def get_resources_inside_folder(user, status=False, folder_id=None):
    if not folder_id:
        folder_id = get_personal_sync_folder_id(user)
    url = url_join(
        get_config('localBackendUrl'),
        f'rest/folders/{folder_id}/children?deleted={status}',
    )
    headers = get_headers(user)
    response = request.get(url, headers)
    assert response.status_code == 200, 'Failed to get the resources inside folder'

    data = json.loads(response.text)
    return data['data']


def get_resource_ids(user, status=False):
    resources = get_resources_inside_folder(user, status)
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


def get_folder_items_count(user, folder_name):
    user = get_username_for_user(user)
    folder_id = get_folder_id_by_path(user, folder_name)
    resources = get_resources_inside_folder(user, folder_id=folder_id)
    return str(len(resources))


def get_resource(user, name, folder_id=None, resource_type=None):
    resources = get_resources_inside_folder(user, folder_id=folder_id)
    for resource in resources:
        if resource['name'] == name and (
            resource_type is None or resource['type'] == resource_type
        ):
            return resource
    return None
