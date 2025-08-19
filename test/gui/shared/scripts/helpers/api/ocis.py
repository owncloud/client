import json

import helpers.api.http_helper as request
from helpers.api.utils import url_join
from helpers.ConfigHelper import get_config


def get_graph_url():
    return url_join(get_config("localBackendUrl"), "graph", "v1.0")


def get_user_id(user):
    url = url_join(get_graph_url(), "users", user)
    response = request.get(url)
    request.assert_http_status(response, 200, f"Failed to get user '{user}'")
    resp_object = response.json()
    return resp_object["id"]


def create_user(username, password, displayname, email):
    url = url_join(get_graph_url(), "users")
    body = json.dumps(
        {
            "onPremisesSamAccountName": username,
            "passwordProfile": {"password": password},
            "displayName": displayname,
            "mail": email,
        }
    )
    response = request.post(url, body)
    request.assert_http_status(response, 201, f"Failed to create user '{username}'")
    resp_object = response.json()
    return {
        "id": resp_object["id"],
        "username": username,
        "password": password,
        "displayname": resp_object["displayName"],
        "email": resp_object["mail"],
    }


def delete_user(user_id):
    url = url_join(get_graph_url(), "users", user_id)
    response = request.delete(url)
    request.assert_http_status(response, 204, "Failed to delete user")
