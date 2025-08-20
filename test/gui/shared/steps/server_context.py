from helpers.api import provisioning, webdav_helper as webdav
from helpers.ConfigHelper import get_config
import helpers.api.external_api as ext

from pageObjects.Toolbar import Toolbar


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


@Given('user "|any|" has been created in the server with default attributes')
def step(context, user):
    provisioning.create_user(user)


@Given('user "|any|" has uploaded file "|any|" to "|any|" in the server')
def step(context, user, file_name, destination):
    if get_config('client_name') != 'ownCloud':
        ext.upload_file(user, destination)
    else:
        webdav.upload_file(user, file_name, destination)
