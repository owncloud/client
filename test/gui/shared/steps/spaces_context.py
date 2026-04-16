from helpers.SpaceHelper import (
    create_space,
    create_space_folder,
    create_space_file,
    add_user_to_space,
    get_file_content,
    resource_exists,
)
from helpers.ConfigHelper import is_owncloud_client
import helpers.api.external_api as ext


@Given('the administrator has created a space "|any|"')
def step(context, space_name):
    if not is_owncloud_client():
        ext.create_space(space_name)
    else:
        create_space(space_name)


@Given('the administrator has created a folder "|any|" in space "|any|"')
def step(context, folder_name, space_name):
    if not is_owncloud_client():
        ext.create_space_folder(space_name, folder_name)
    else:
        create_space_folder(space_name, folder_name)


@Given(
    'the administrator has uploaded a file "|any|" with content "|any|" inside space "|any|"'
)
def step(context, file_name, content, space_name):
    if not is_owncloud_client():
        ext.upload_space_file(space_name, file_name, content)
    else:
        create_space_file(space_name, file_name, content)


@Given('the administrator has added user "|any|" to space "|any|" with role "|any|"')
def step(context, user, space_name, role):
    if not is_owncloud_client():
        ext.add_user_to_space(user, space_name, role)
    else:
        add_user_to_space(user, space_name, role)


@Then(
    'as "|any|" the file "|any|" in the space "|any|" should have content "|any|" in the server'
)
def step(context, user, file_name, space_name, content):
    if not is_owncloud_client():
        downloaded_content = ext.get_file_content(user, file_name, space_name)
    else:
        downloaded_content = get_file_content(space_name, file_name, user)
    test.compare(downloaded_content, content, 'Comparing file content')


@Then(
    r'as "([^"]*)" the space "([^"]*)" (should|should not) have (?:folder|file) "([^"]*)" in the server',
    regexp=True,
)
def step(context, user, space_name, should_or_should_not, resource_name):
    expected = should_or_should_not == 'should'
    if not is_owncloud_client():
        exists = ext.resource_exists(user, resource_name, space_name)
    else:
        exists = resource_exists(space_name, resource_name, user)
    test.compare(exists, expected, 'Resource exists')


@When('user "|any|" uploads a file "|any|" with content "|any|" inside space "|any|"')
def step(context, user, file_name, file_content, space_name):
    ext.upload_space_file(space_name, file_name, file_content, user)
