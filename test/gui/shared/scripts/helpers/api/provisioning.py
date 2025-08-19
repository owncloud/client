from helpers.api import ocis
from helpers.ConfigHelper import get_config
from helpers import UserHelper

created_users = {}


def create_user(username):
    if get_config('client_name') != 'ownCloud':
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
        if get_config('client_name') == 'ownCloud':
            ocis.delete_user(user_info['id'])
        del created_users[username]
