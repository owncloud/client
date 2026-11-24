# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Python proxies for objects that live in the application under test."""

from harness.client import ProbeError
from harness.selectors import HandleRef, encode_value

# Error codes that mean a handle no longer refers to a live object.
_STALE = ("DeadObject", "NoSuchObject")


def request(client, method, **params):
    """Send a request, turning stale-handle errors into RuntimeError."""
    try:
        return client.request(method, **params)
    except ProbeError as err:
        if err.code in _STALE:
            raise RuntimeError(f"object no longer exists ({err.message})") from err
        raise


def wrap(client, value):
    """Wrap object references in a probe result, recursively."""
    if isinstance(value, dict):
        if "$handle" in value:
            return RemoteObject(client, value["$handle"], value.get("$class"))
        return {key: wrap(client, item) for key, item in value.items()}
    if isinstance(value, list):
        return [wrap(client, item) for item in value]
    return value


class RemoteMethod:
    def __init__(self, obj, name):
        self._obj = obj
        self._name = name

    def __call__(self, *args):
        obj = self._obj
        result = request(
            obj.client,
            "call",
            handle=obj.handle,
            name=self._name,
            args=[encode_value(arg) for arg in args],
        )
        return wrap(obj.client, result)

    def __repr__(self):
        return f"<RemoteMethod {self._name} of {self._obj!r}>"


class RemoteObject(HandleRef):
    """An object reference (``$handle``) in the application under test."""

    __slots__ = ("_client", "_handle", "_class")

    def __init__(self, client, handle, cls=None):
        object.__setattr__(self, "_client", client)
        object.__setattr__(self, "_handle", handle)
        object.__setattr__(self, "_class", cls)

    @property
    def client(self):
        return self._client

    @property
    def handle(self):
        return self._handle

    @property
    def class_name(self):
        return self._class

    def is_null(self):
        return self._handle is None

    def _get(self, name):
        """Read a property; raises ProbeError('NoSuchProperty') if absent."""
        if self.is_null():
            raise ProbeError("NoSuchProperty", f"null object has no '{name}'")
        result = request(self._client, "get", handle=self._handle, name=name)
        return wrap(self._client, result)

    def __getattr__(self, name):
        if name.startswith("__") and name.endswith("__"):
            raise AttributeError(name)
        try:
            return self._get(name)
        except ProbeError as err:
            if err.code != "NoSuchProperty":
                raise
            if err.is_method:
                return RemoteMethod(self, name)
            raise AttributeError(f"{self!r} has no attribute '{name}'") from err

    def __getitem__(self, name):
        try:
            return self._get(name)
        except ProbeError as err:
            if err.code != "NoSuchProperty":
                raise
            raise KeyError(name) from err

    def __setattr__(self, name, value):
        raise AttributeError(
            f"cannot set '{name}': remote properties are read-only, call a setter"
        )

    def is_alive(self):
        """Whether the handle still refers to a live object (one round trip)."""
        if self.is_null():
            return False
        try:
            request(self._client, "get", handle=self._handle, name="objectName")
        except RuntimeError:
            return False
        except ProbeError:
            pass
        return True

    def __str__(self):
        # Squish prints null and dead objects alike; the suite compares to it.
        return repr(self) if self.is_alive() else "<null>"

    def __repr__(self):
        if self.is_null():
            return "<RemoteObject null>"
        return f"<RemoteObject {self._class or '?'} #{self._handle}>"

    def __bool__(self):
        return not self.is_null()

    def __eq__(self, other):
        if not isinstance(other, RemoteObject):
            return NotImplemented
        return self._handle == other.handle

    def __hash__(self):
        return hash(self._handle)
