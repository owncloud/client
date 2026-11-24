# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's ``object`` module."""

from harness.client import current_client
from harness.proxy import RemoteObject, request, wrap
from harness.selectors import encode


def children(obj):
    """The direct children of a live object."""
    if not isinstance(obj, RemoteObject):
        raise TypeError(f"object.children() needs an object, got {obj!r}")
    return wrap(obj.client, request(obj.client, "children", handle=obj.handle))


def exists(sel):
    """Whether the selector matches an object now, without waiting."""
    if isinstance(sel, RemoteObject):
        return sel.is_alive()
    client = current_client()
    return bool(
        wrap(client, request(client, "find", selector=encode(sel), ready=False))
    )
