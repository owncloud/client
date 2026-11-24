# SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
# SPDX-License-Identifier: MIT
# pylint: disable=redefined-outer-name

import time

import pytest

import object  # pylint: disable=redefined-builtin
import squish
from objectmaphelper import RegularExpression, Wildcard
from fake_probe import FakeProbe
from harness import client as client_module
from harness.proxy import RemoteObject
from harness.selectors import encode

TREE = {"name": "tree", "type": "QTreeView", "visible": 1}


@pytest.fixture
def fake():
    probe = FakeProbe()
    client_module.set_current_client(probe.client)
    yield probe
    client_module.set_current_client(None)
    probe.close()


@pytest.fixture
def short_timeout(monkeypatch):
    monkeypatch.setattr(squish.testSettings, "waitForObjectTimeout", 300)


def test_wait_for_object_returns_once_ready_on_third_poll(fake):
    tree = fake.add("QTreeView", props={"name": "tree"}, visible=False)

    def make_ready(method, _):
        if method == "find" and len(fake.sent("find")) == 3:
            tree.visible = True

    fake.before_request = make_ready
    obj = squish.waitForObject(TREE)
    assert obj == RemoteObject(fake.client, tree.handle)
    finds = fake.sent("find")
    assert len(finds) == 3
    assert all(p["ready"] is True for p in finds)


@pytest.mark.usefixtures("short_timeout")
def test_wait_for_object_times_out_with_lookup_error(fake):
    start = time.monotonic()
    with pytest.raises(LookupError):
        squish.waitForObject(TREE)
    elapsed = time.monotonic() - start
    assert 0.3 <= elapsed < 2
    assert len(fake.sent("find")) >= 2


@pytest.mark.usefixtures("fake")
def test_wait_for_object_honours_explicit_timeout():
    start = time.monotonic()
    with pytest.raises(LookupError):
        squish.waitForObject(TREE, 200)
    assert time.monotonic() - start < 2


@pytest.mark.usefixtures("short_timeout")
def test_wait_for_object_exists_accepts_hidden_object(fake):
    tree = fake.add("QTreeView", props={"name": "tree"}, visible=False)
    obj = squish.waitForObjectExists({"name": "tree", "type": "QTreeView"})
    assert obj == RemoteObject(fake.client, tree.handle)
    assert fake.sent("find")[-1]["ready"] is False


def test_wait_for_object_passes_raw_handles_through(fake):
    tree = fake.add("QTreeView")
    proxy = RemoteObject(fake.client, tree.handle, "QTreeView")
    assert squish.waitForObject(proxy) is proxy
    assert not fake.sent("find")
    with pytest.raises(LookupError):
        squish.waitForObject(RemoteObject(fake.client, None))


def test_find_object_raises_immediately(fake):
    with pytest.raises(LookupError):
        squish.findObject(TREE)
    assert len(fake.sent("find")) == 1
    assert fake.sent("find")[0]["ready"] is False


def test_find_all_objects_and_children(fake):
    stack = fake.add("QStackedWidget")
    first = fake.add("QLabel", parent=stack)
    second = fake.add("QLabel", parent=stack)
    labels = squish.findAllObjects({"type": "QLabel"})
    assert labels == [
        RemoteObject(fake.client, first.handle),
        RemoteObject(fake.client, second.handle),
    ]
    assert object.children(RemoteObject(fake.client, stack.handle)) == labels


def test_object_exists_does_not_wait(fake):
    fake.add("QTreeView", props={"name": "tree"})
    assert object.exists(TREE) is True
    assert object.exists({"name": "other"}) is False
    assert len(fake.sent("find")) == 2


def test_encode_regex_and_handle():
    proxy = RemoteObject(None, 7, "QTreeView")
    selector = {"text": RegularExpression("^a$"), "container": proxy}
    assert encode(selector) == {"text": {"$regex": "^a$"}, "container": {"$handle": 7}}


def test_encode_wildcard_and_nested():
    selector = {"window": {"title": Wildcard("Set*")}, "visible": 1}
    assert encode(selector) == {
        "window": {"title": {"$wildcard": "Set*"}},
        "visible": 1,
    }


def test_proxy_reads_unicode_text(fake):
    label = fake.add("QLabel", props={"text": "Grüße 🙂"})
    assert RemoteObject(fake.client, label.handle).text == "Grüße 🙂"


def test_proxy_calls_methods_and_wraps_results(fake):
    model = fake.add("QStandardItemModel", methods={"rowCount": lambda: 3})
    view = fake.add("QTreeView", methods={"model": lambda: model})
    proxy = RemoteObject(fake.client, view.handle)
    assert proxy.model().rowCount() == 3
    assert fake.sent("call")[0] == {"handle": view.handle, "name": "model", "args": []}


def test_any_probe_method_is_callable(fake):
    view = fake.add("QTableView", methods={"columnCount": lambda: 4})
    proxy = RemoteObject(fake.client, view.handle)
    assert proxy.columnCount() == 4
    assert hasattr(proxy, "columnCount") is True
    assert hasattr(proxy, "rowCount") is False
    assert fake.sent("get")[0] == {"handle": view.handle, "name": "columnCount"}


def test_proxy_item_access(fake):
    header = fake.add("HeaderViewItem", props={"section": 1})
    assert RemoteObject(fake.client, header.handle)["section"] == 1


def test_null_proxy():
    null = RemoteObject(None, None)
    assert str(null) == "<null>"
    assert not null


def test_active_modal_widget_null_and_live(fake):
    assert str(squish.QApplication.activeModalWidget()) == "<null>"
    dialog = fake.add("QDialog", props={"objectName": "dlg"})
    fake.modal = dialog
    modal = squish.QApplication.activeModalWidget()
    assert modal
    assert modal.objectName == "dlg"


def test_hasattr_false_for_unknown_name(fake):
    label = fake.add("QLabel", props={"text": "x"})
    proxy = RemoteObject(fake.client, label.handle)
    assert hasattr(proxy, "text") is True
    assert hasattr(proxy, "nope") is False


def test_returned_refs_are_wrapped(fake):
    action = fake.add("QAction", props={"objectName": "act"})
    button = fake.add("QToolButton", props={"defaultAction": action})
    proxy = RemoteObject(fake.client, button.handle)
    assert isinstance(proxy.defaultAction, RemoteObject)
    assert proxy.defaultAction.objectName == "act"


def test_dead_handle_raises_runtime_error(fake):
    label = fake.add("QLabel", props={"text": "x"})
    proxy = RemoteObject(fake.client, label.handle)
    fake.kill(label)
    with pytest.raises(RuntimeError, match="object no longer exists"):
        _ = proxy.text
    with pytest.raises(RuntimeError, match="object no longer exists"):
        squish.mouseClick(proxy)
    assert str(proxy) == "<null>"


def test_equality_and_hash_by_handle(fake):
    first = RemoteObject(fake.client, 5, "QLabel")
    same = RemoteObject(fake.client, 5)
    other = RemoteObject(fake.client, 6)
    assert first == same
    assert hash(first) == hash(same)
    assert first != other
    assert len({first, same, other}) == 2


def test_mouse_click_argument_order(fake):
    obj = fake.add("QTreeView")
    proxy = RemoteObject(fake.client, obj.handle)
    squish.mouseClick(proxy, 11, 11, squish.Qt.NoModifier, squish.Qt.LeftButton)
    params = dict(fake.sent("mouseClick")[0])
    assert params.pop("handle") == obj.handle
    assert params == {"x": 11, "y": 11, "modifiers": 0, "button": 1, "clicks": 1}


def test_mouse_click_defaults_and_double_click(fake):
    obj = fake.add("QPushButton", props={"name": "ok"})
    squish.mouseClick(squish.waitForObject({"name": "ok"}))
    squish.doubleClick(RemoteObject(fake.client, obj.handle))
    squish.clickButton(RemoteObject(fake.client, obj.handle))
    single, double, button = fake.sent("mouseClick")
    assert single == {
        "handle": obj.handle,
        "modifiers": 0,
        "button": squish.Qt.LeftButton,
        "clicks": 1,
    }
    assert double["clicks"] == 2
    assert button == single


def test_typing_and_activation(fake):
    edit = fake.add("QLineEdit")
    proxy = RemoteObject(fake.client, edit.handle)
    squish.type(proxy, "Grüße 🙂")
    squish.nativeType("<Return>")
    squish.activateItem(proxy)
    assert fake.sent("typeText") == [
        {"handle": edit.handle, "text": "Grüße 🙂"},
        {"text": "<Return>"},
    ]
    assert fake.sent("activateItem") == [{"handle": edit.handle}]


def test_wait_for_object_item_selector_shapes(fake):
    tree = fake.add("QTreeWidget", props={"name": "folderTreeWidget"})
    item = fake.add("QModelIndex", parent=tree, props={"text": "Personal"})
    menu = fake.add("QMenu", props={"name": "menu"})
    action = fake.add("QAction", parent=menu, props={"text": "Remove"})
    tree_sel = {"name": "folderTreeWidget", "type": "QTreeWidget"}
    menu_sel = {"name": "menu", "type": "QMenu"}
    assert squish.waitForObjectItem(tree_sel, "Personal") == RemoteObject(
        fake.client, item.handle
    )
    assert squish.waitForObjectItem(menu_sel, "Remove") == RemoteObject(
        fake.client, action.handle
    )
    tree_find, menu_find = fake.sent("find")
    assert tree_find["selector"] == {
        "container": tree_sel,
        "text": "Personal",
        "type": "QModelIndex",
    }
    assert menu_find["selector"] == {
        "container": menu_sel,
        "text": "Remove",
        "type": "QAction",
    }
    assert tree_find["ready"] is True


def test_clipboard_screenshot_snooze(fake):
    fake.clipboard = "https://example.org/x"
    assert squish.getClipboardText() == "https://example.org/x"
    squish.saveDesktopScreenshot("/tmp/shot.png")
    assert fake.sent("screenshot") == [{"path": "/tmp/shot.png"}]
    start = time.monotonic()
    squish.snooze(0.1)
    assert time.monotonic() - start >= 0.1


def test_wait_for_callable_and_string():
    calls = []

    def ready():
        calls.append(1)
        return len(calls) >= 2

    assert squish.waitFor(ready, 2000) is True
    flag = True
    assert squish.waitFor("flag", 100) is True
    assert flag
    start = time.monotonic()
    assert squish.waitFor(lambda: False, 200) is False
    assert 0.2 <= time.monotonic() - start < 2


def test_qt_constants():
    qt = squish.Qt
    assert (qt.NoModifier, qt.LeftButton, qt.RightButton) == (0, 1, 2)
    assert (qt.ShiftModifier, qt.ControlModifier, qt.AltModifier) == (
        0x02000000,
        0x04000000,
        0x08000000,
    )


def test_objectmaphelper_exports():
    import objectmaphelper  # pylint: disable=import-outside-toplevel

    assert sorted(objectmaphelper.__all__) == ["RegularExpression", "Wildcard"]
    assert RegularExpression("^a$").pattern == "^a$"
    assert Wildcard("a*").pattern == "a*"


def test_no_current_client_is_a_clear_error():
    client_module.set_current_client(None)
    with pytest.raises(RuntimeError, match="no application"):
        squish.findObject(TREE)
