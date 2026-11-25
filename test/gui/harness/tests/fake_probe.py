# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
"""In-process fake of the GUI test probe.

It speaks the line-based JSON wire protocol over one end of a socketpair and
answers from a dict-based object tree. Tests drive the harness through a real
ProbeClient connected to the other end.
"""

import fnmatch
import json
import re
import socket
import threading

from harness.client import ProbeClient


class FakeObject:
    # pylint: disable=too-many-instance-attributes,too-few-public-methods
    def __init__(self, handle, cls, parent, props, methods, visible, enabled):
        self.handle = handle
        self.cls = cls
        self.parent = parent
        self.props = dict(props or {})
        self.methods = dict(methods or {})
        self.visible = visible
        self.enabled = enabled
        self.alive = True
        self.children = []

    def ancestors(self):
        node = self.parent
        while node is not None:
            yield node
            node = node.parent


class FakeError(Exception):
    def __init__(self, code, message, **extra):
        super().__init__(message)
        self.code = code
        self.message = message
        self.extra = extra


class FakeProbe:  # pylint: disable=too-many-instance-attributes
    """Holds the object tree and serves requests on a background thread."""

    def __init__(self, sock=None, pid=4242):
        """Serve on ``sock``; without one, connect ``self.client`` to a pair."""
        self._objects = {}
        self._roots = []
        self._next_handle = 1
        self.requests = []
        self.before_request = None
        self.clipboard = ""
        self.modal = None
        self.pid = pid
        if sock is None:
            self._server, client_sock = socket.socketpair()
            self.client = ProbeClient(client_sock)
        else:
            self._server, self.client = sock, None
        self._thread = threading.Thread(target=self._serve, daemon=True)
        self._thread.start()

    # -- tree building -----------------------------------------------------

    def add(
        self,
        cls,
        parent=None,
        props=None,
        methods=None,
        visible=True,
        enabled=True,
        handle=None,
    ):
        if handle is None:
            handle = self._next_handle
        self._next_handle = max(self._next_handle, handle) + 1
        obj = FakeObject(handle, cls, parent, props, methods, visible, enabled)
        self._objects[handle] = obj
        if parent is None:
            self._roots.append(obj)
        else:
            parent.children.append(obj)
        return obj

    @staticmethod
    def kill(obj):
        obj.alive = False

    def exit(self):
        """Close the probe's end, as an application that exits does."""
        self._server.shutdown(socket.SHUT_RDWR)
        self._server.close()
        self._thread.join(timeout=2)

    def close(self):
        if self.client is not None:
            self.client.close()
        self._server.close()
        self._thread.join(timeout=2)

    def sent(self, method):
        """Params of every request with the given method, in order."""
        return [params for name, params in self.requests if name == method]

    # -- protocol ----------------------------------------------------------

    def _serve(self):
        with self._server.makefile("rb") as reader:
            for line in reader:
                request = json.loads(line.decode("utf-8"))
                response = {"id": request["id"]}
                try:
                    response["result"] = self._dispatch(
                        request["method"], request.get("params", {})
                    )
                except FakeError as err:
                    response["error"] = {
                        "code": err.code,
                        "message": err.message,
                        **err.extra,
                    }
                data = json.dumps(response, ensure_ascii=False) + "\n"
                try:
                    self._server.sendall(data.encode("utf-8"))
                except OSError:
                    return

    def _dispatch(self, method, params):
        self.requests.append((method, params))
        if self.before_request is not None:
            self.before_request(method, params)
        name = re.sub("([A-Z])", lambda m: "_" + m.group(1).lower(), method)
        if (handler := getattr(self, "_m_" + name, None)) is None:
            raise FakeError("BadRequest", f"unknown method {method}")
        return handler(params)

    @staticmethod
    def _ref(obj):
        if obj is None:
            return {"$handle": None}
        return {"$handle": obj.handle, "$class": obj.cls}

    def _to_json(self, value):
        if isinstance(value, FakeObject):
            return self._ref(value)
        if isinstance(value, (list, tuple)):
            return [self._to_json(v) for v in value]
        return value

    def _resolve(self, ref):
        handle = ref["$handle"] if isinstance(ref, dict) else ref
        if (obj := self._objects.get(handle)) is None:
            raise FakeError("NoSuchObject", f"unknown handle {handle}")
        if not obj.alive:
            raise FakeError("DeadObject", f"handle {handle} is dead")
        return obj

    def _all(self):
        stack = list(reversed(self._roots))
        while stack:
            obj = stack.pop()
            if obj.alive:
                yield obj
                stack.extend(reversed(obj.children))

    def _value_matches(self, want, have):
        if isinstance(want, dict) and "$regex" in want:
            return re.search(want["$regex"], str(have)) is not None
        if isinstance(want, dict) and "$wildcard" in want:
            return fnmatch.fnmatchcase(str(have), want["$wildcard"])
        if isinstance(want, bool) or isinstance(have, bool):
            return str(want).lower() in (str(have).lower(), str(int(bool(have))))
        return str(want) == str(have)

    def _scope_matches(self, obj, want):
        if "$handle" in want:
            return obj.handle == want["$handle"]
        return self._matches(obj, want)

    def _matches(self, obj, selector):
        for key, want in selector.items():
            if key == "type":
                ok = obj.cls == want
            elif key == "visible":
                ok = self._value_matches(want, obj.visible)
            elif key == "container":
                ok = any(self._scope_matches(a, want) for a in obj.ancestors())
            elif key in obj.props:
                ok = self._value_matches(want, obj.props[key])
            else:
                ok = False
            if not ok:
                return False
        return True

    def _m_hello(self, _):
        return {"pid": self.pid, "qtVersion": "6.8.0"}

    def _m_find(self, params):
        if "$handle" in params["selector"]:  # a handle as the whole selector
            obj = self._resolve(params["selector"])
            if params.get("ready") and not (obj.visible and obj.enabled):
                return self._ref(None)
            return self._ref(obj)
        for obj in self._all():
            if self._matches(obj, params["selector"]):
                if params.get("ready") and not (obj.visible and obj.enabled):
                    continue
                return self._ref(obj)
        return self._ref(None)

    def _m_find_all(self, params):
        return [
            self._ref(o) for o in self._all() if self._matches(o, params["selector"])
        ]

    def _m_children(self, params):
        obj = self._resolve(params["handle"])
        return [self._ref(c) for c in obj.children if c.alive]

    def _m_get(self, params):
        obj = self._resolve(params["handle"])
        name = params["name"]
        if name == "visible":
            return obj.visible
        if name not in obj.props:
            raise FakeError(
                "NoSuchProperty",
                f"{obj.cls} has no property {name}",
                isMethod=name in obj.methods,
            )
        return self._to_json(obj.props[name])

    def _m_call(self, params):
        obj = self._resolve(params["handle"])
        if (method := obj.methods.get(params["name"])) is None:
            raise FakeError(
                "MethodNotAllowed", f"{obj.cls}::{params['name']} is not allowed"
            )
        return self._to_json(method(*params.get("args", [])))

    def _m_mouse_click(self, params):
        self._resolve(params["handle"])

    def _m_click_button(self, params):
        self._resolve(params["handle"])

    def _m_type_text(self, params):
        if params.get("handle") is not None:
            self._resolve(params["handle"])

    def _m_activate_item(self, params):
        self._resolve(params["handle"])

    def _m_clipboard_text(self, _):
        return self.clipboard

    def _m_screenshot(self, _):
        return None

    def _m_active_modal_widget(self, _):
        return self._ref(self.modal)

    def _m_release(self, _):
        self._objects.clear()
        self._roots.clear()
