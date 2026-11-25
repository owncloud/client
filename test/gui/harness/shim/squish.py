# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""Squish's ``squish`` module, implemented on top of the probe."""

# Squish's names are dictated by the suite: camelCase functions and ``type``.
# pylint: disable=invalid-name,redefined-builtin

import importlib
import inspect
import time

from harness.client import current_client
from harness.proxy import RemoteObject, request, wrap
from harness.selectors import encode, encode_value
from harness.settings import test_settings

testSettings = test_settings

_POLL_INTERVAL = 0.1


class Qt:  # pylint: disable=too-few-public-methods
    NoModifier = 0
    LeftButton = 1
    RightButton = 2
    ShiftModifier = 0x02000000
    ControlModifier = 0x04000000
    AltModifier = 0x08000000


class QApplication:  # pylint: disable=too-few-public-methods
    @staticmethod
    def activeModalWidget():
        client = current_client()
        return wrap(client, request(client, "activeModalWidget"))


def _timeout_ms(timeout):
    return testSettings.waitForObjectTimeout if timeout is None else timeout


def _poll_find(sel, ready, timeout):
    """Poll ``find`` until it matches; an object is found as soon as it is ready."""
    if isinstance(sel, RemoteObject):
        if sel.is_null():
            raise LookupError("cannot wait for the null object")
        if not ready:
            return sel
        client, encoded = sel.client, encode_value(sel)  # the handle as selector
    else:
        client, encoded = current_client(), encode(sel)
    timeout = _timeout_ms(timeout)
    deadline = time.monotonic() + timeout / 1000
    while True:
        try:
            found = wrap(client, request(client, "find", selector=encoded, ready=ready))
        except (RuntimeError, ConnectionError, TimeoutError) as err:
            # a dead object, or its application closed or stopped answering
            if not isinstance(sel, RemoteObject):
                raise
            raise LookupError(f"cannot wait for {sel!r}: {err}") from err
        if found:
            # a ready object is returned as it was given
            return sel if isinstance(sel, RemoteObject) else found
        if (remaining := deadline - time.monotonic()) <= 0:
            state = "ready" if ready else "existing"
            raise LookupError(f"no {state} object after {timeout} ms: {sel!r}")
        time.sleep(min(_POLL_INTERVAL, remaining))


def waitForObject(sel, timeout=None):
    """Wait until the selector matches a visible and enabled object.

    An object found before is waited for until it is visible and enabled.
    """
    return _poll_find(sel, True, timeout)


def waitForObjectExists(sel, timeout=None):
    """Wait until the selector matches an object, visible or not.

    An object found before is returned at once.
    """
    return _poll_find(sel, False, timeout)


def _item_type(container):
    # The suite calls this on item views (items are QModelIndex) and on QMenus,
    # whose items are their QActions.
    if isinstance(container, RemoteObject):
        cls = container.class_name
    else:
        cls = container.get("type")
    return "QAction" if cls == "QMenu" else "QModelIndex"


def waitForObjectItem(container, itemText, timeout=None):
    """Wait for the item with the given text inside a view or menu."""
    selector = {"container": container, "text": itemText}
    selector["type"] = _item_type(container)
    return _poll_find(selector, True, timeout)


def findObject(sel):
    """Find an object now; raise LookupError if there is none."""
    return _poll_find(sel, False, 0)


def findAllObjects(sel):
    client = current_client()
    return wrap(client, request(client, "findAll", selector=encode(sel)))


def _resolve(obj):
    return obj if isinstance(obj, RemoteObject) else waitForObject(obj)


def _click(obj, x, y, modifiers, button, clicks):
    obj = _resolve(obj)
    params = {"handle": obj.handle}
    if x is not None and y is not None:
        params["x"] = x
        params["y"] = y
    params.update(modifiers=modifiers, button=button, clicks=clicks)
    request(obj.client, "mouseClick", **params)


def mouseClick(obj, x=None, y=None, modifiers=0, button=Qt.LeftButton):
    _click(obj, x, y, modifiers, button, 1)


def doubleClick(obj, x=None, y=None, modifiers=0, button=Qt.LeftButton):
    _click(obj, x, y, modifiers, button, 2)


def clickButton(obj):
    """Click where the button accepts clicks; a stretched radio button or check box
    ignores a click at its centre."""
    obj = _resolve(obj)
    request(obj.client, "clickButton", handle=obj.handle)


def type(obj, text):
    obj = _resolve(obj)
    request(obj.client, "typeText", handle=obj.handle, text=str(text))


def nativeType(text):
    """Type into whatever widget has the keyboard focus."""
    client = current_client()
    request(client, "typeText", text=str(text))


def activateItem(obj):
    obj = _resolve(obj)
    request(obj.client, "activateItem", handle=obj.handle)


def getClipboardText():
    client = current_client()
    return request(client, "clipboardText")


def saveDesktopScreenshot(path):
    client = current_client()
    request(client, "screenshot", path=str(path))


def snooze(seconds):
    time.sleep(float(seconds))


def waitFor(cond, timeout_ms=None):
    """Poll ``cond`` until it is true; False after ``timeout_ms`` (None: forever).

    A string condition is evaluated in the caller's frame.
    """
    check = cond
    if isinstance(cond, str):
        frame = inspect.currentframe().f_back
        cond_globals, cond_locals = frame.f_globals, frame.f_locals
        del frame

        def check_expression():
            return eval(cond, cond_globals, cond_locals)  # pylint: disable=eval-used

        check = check_expression

    deadline = None if timeout_ms is None else time.monotonic() + timeout_ms / 1000
    while True:
        if check():
            return True
        if deadline is None:
            time.sleep(_POLL_INTERVAL)
            continue
        if (remaining := deadline - time.monotonic()) <= 0:
            return False
        time.sleep(min(_POLL_INTERVAL, remaining))


def _aut():
    # Imported on use: harness.aut needs the runner's configuration.
    return importlib.import_module("harness.aut")


def startApplication(*args, **kwargs):
    return _aut().startApplication(*args, **kwargs)


def applicationContextList():
    return _aut().applicationContextList()


def currentApplicationContext():
    return _aut().currentApplicationContext()
