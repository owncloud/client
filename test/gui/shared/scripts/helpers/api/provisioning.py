from helpers.api import ocis
from helpers.ConfigHelper import is_owncloud_client
from helpers import UserHelper

created_users = {}


def create_user(username):
    if not is_owncloud_client():
        user = UserHelper.test_users[username]
        created_users[username] = {
            'username': user.username,
            'password': user.password,
            'displayname': user.displayname,
            'email': user.email,
        }
        return

    if username in UserHelper.test_users:
        user = UserHelper.test_users[username]
    else:
        user = UserHelper.User(
            username=username,
            displayname=username,
            email=f'{username}@mail.com',
            password=UserHelper.get_default_password(),
        )

    user_info = ocis.create_user(
        user.username,
        user.password,
        user.displayname,
        user.email,
    )
    created_users[username] = user_info


def delete_created_users():
    for username, user_info in list(created_users.items()):
        if is_owncloud_client():
            ocis.delete_user(user_info['id'])
        del created_users[username]
