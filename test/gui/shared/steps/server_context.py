from helpers.api import provisioning, sharing_helper, webdav_helper as webdav
import helpers.api.oc10 as oc
from helpers.ConfigHelper import get_config
import helpers.api.external_api as ext

from pageObjects.Toolbar import Toolbar


@Given('app "|any|" has been "|any|" in the server')
def step(context, app_name, action):
    oc.setup_app(app_name, action)


@Then(
    r'^as "([^"].*)" (?:file|folder) "([^"].*)" should not exist in the server',
    regexp=True,
)
def step(context, user_name, resource_name):
    if get_config('client_name') != 'ownCloud':
        resource_exists = ext.resource_exists(user_name, resource_name)
    else:
        resource_exists = webdav.resource_exists(user_name, resource_name)
    test.compare(
        resource_exists,
        False,
        f"Resource '{resource_name}' should not exist, but does",
    )


@Then(
    r'^as "([^"].*)" (?:file|folder) "([^"].*)" should exist in the server', regexp=True
)
def step(context, user_name, resource_name):
    if get_config('client_name') != 'ownCloud':
        resource_exists = ext.resource_exists(user_name, resource_name)
    else:
        resource_exists = webdav.resource_exists(user_name, resource_name)
    test.compare(
        resource_exists,
        True,
        f"Resource '{resource_name}' should exist, but does not",
    )


@Then('as "|any|" the file "|any|" should have the content "|any|" in the server')
def step(context, user_name, file_name, content):
    if get_config('client_name') != 'ownCloud':
        text_content = ext.get_file_content(user_name, file_name)
    else:
        text_content = webdav.get_file_content(user_name, file_name)
    test.compare(
        text_content,
        content,
        f"File '{file_name}' should have content '{content}' but found '{text_content}'",
    )


@Then(
    r'as user "([^"].*)" the (?:file|folder) "([^"].*)" should have a public link in the server',
    regexp=True,
)
def step(context, user_name, resource_name):
    has_link_share = sharing_helper.has_public_link_share(user_name, resource_name)
    test.compare(
        has_link_share,
        True,
        f"Resource '{resource_name}' does not have public link share",
    )


@Then(
    r'as user "([^"].*)" the (?:file|folder) "([^"].*)" should not have any public link in the server',
    regexp=True,
)
def step(context, user_name, resource_name):
    has_link_share = sharing_helper.has_public_link_share(user_name, resource_name)
    test.compare(
        has_link_share, False, f"Resource '{resource_name}' have public link share"
    )


@Then(
    # pylint: disable=line-too-long
    r'the public should be able to download the (?:file|folder) "([^"].*)" without password from the last created public link by "([^"].*)" in the server',
    regexp=True,
)
def step(context, resource_name, link_creator):
    downloaded = sharing_helper.download_last_public_link_resource(
        link_creator, resource_name
    )
    test.compare(downloaded, True, 'Could not download public share')


@Then(
    # pylint: disable=line-too-long
    r'the public should not be able to download the (?:file|folder) "([^"].*)" from the last created public link by "([^"].*)" in the server',
    regexp=True,
)
def step(context, resource_name, link_creator):
    downloaded = sharing_helper.download_last_public_link_resource(
        link_creator, resource_name
    )
    test.compare(downloaded, False, 'Could download public share')


@Then(
    # pylint: disable=line-too-long
    r'the public should be able to download the (?:file|folder) "([^"].*)" with password "([^"].*)" from the last created public link by "([^"].*)" in the server',
    regexp=True,
)
def step(context, resource_name, public_link_password, link_creator):
    downloaded = sharing_helper.download_last_public_link_resource(
        link_creator, resource_name, public_link_password
    )
    test.compare(downloaded, True, 'Could not download public share')


@Then(
    r'as user "([^"].*)" folder "([^"].*)" should contain "([^"].*)" items in the server',
    regexp=True,
)
def step(context, user_name, folder_name, items_number):
    if get_config('client_name') != 'ownCloud':
        total_items = ext.get_folder_items_count(user_name, folder_name)
    else:
        total_items = webdav.get_folder_items_count(user_name, folder_name)
    test.compare(
        total_items, items_number, f'Folder should contain {items_number} items'
    )


@Given('user "|any|" has created folder "|any|" in the server')
def step(context, user, folder_name):
    if get_config('client_name') != 'ownCloud':
        ext.create_folder(user, folder_name)
    else:
        webdav.create_folder(user, folder_name)


@Given('user "|any|" has uploaded file with content "|any|" to "|any|" in the server')
def step(context, user, file_content, file_name):
    if get_config('client_name') != 'ownCloud':
        ext.upload_file(user, file_name, file_content)
    else:
        webdav.create_file(user, file_name, file_content)


@When('the user clicks on the settings tab')
def step(context):
    Toolbar.open_settings_tab()


@When('user "|any|" uploads file with content "|any|" to "|any|" in the server')
def step(context, user, file_content, file_name):
    if get_config('client_name') != 'ownCloud':
        ext.upload_file(user, file_name, file_content)
    else:
        webdav.create_file(user, file_name, file_content)


@When('user "|any|" deletes the folder "|any|" in the server')
def step(context, user, folder_name):
    if get_config('client_name') != 'ownCloud':
        ext.delete_resource(user, folder_name)
    else:
        webdav.delete_resource(user, folder_name)


@Given('group "|any|" has been created in the server')
def step(context, group_name):
    provisioning.create_group(group_name)


@Given('user "|any|" has been added to group "|any|" in the server')
def step(context, user, group_name):
    provisioning.add_user_to_group(user, group_name)


@Given('user "|any|" has been created in the server with default attributes')
def step(context, user):
    provisioning.create_user(user)


@Given(
    # pylint: disable=line-too-long
    r'user "([^"].*)" has shared (?:file|folder) "([^"].*)" in the server with (user|group) "([^"].*)" with "([^"].*)" permission(?:s)?',
    regexp=True,
)
def step(context, user, resource, receiver_type, receiver, permissions):
    sharing_helper.share_resource(user, resource, receiver, permissions, receiver_type)


@Given('user "|any|" has created the following public link share in the server')
def step(context, user):
    data = {
        'resource': None,
        'permissions': None,
        'name': None,
        'password': None,
        'expireDate': None,
    }
    for row in context.table:
        key, value = row
        if key in data:
            data[key] = value
    sharing_helper.create_link_share(
        user,
        data['resource'],
        data['permissions'],
        data['name'],
        data['password'],
        data['expireDate'],
    )


@Then('user "|any|" should have a share with these details in the server:')
def step(context, user):
    path = None
    share_type = None
    share_with = None
    for key, value in context.table[1:]:
        if key == 'path':
            path = value
        elif key == 'share_type':
            share_type = value
        elif key == 'share_with':
            share_with = value

    share = sharing_helper.get_share(user, path, share_type, share_with)

    for key, value in context.table[1:]:
        if key == 'permissions':
            value = sharing_helper.get_permission_value(value)
        if key == 'share_type':
            value = sharing_helper.share_types[value]
        assert share.get(key) == value, 'Key value did not match'


@Given('user "|any|" has uploaded file "|any|" to "|any|" in the server')
def step(context, user, file_name, destination):
    if get_config('client_name') != 'ownCloud':
        ext.upload_file(user, destination)
    else:
        webdav.upload_file(user, file_name, destination)
