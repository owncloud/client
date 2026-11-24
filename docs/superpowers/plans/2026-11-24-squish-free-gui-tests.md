# Squish-free GUI Tests Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Run the existing `test/gui` suite unchanged on GitHub Actions without Squish. A probe compiled
into the client and a Squish-compatible Python harness take Squish's place.

**Architecture:**
- **Probe:** a C++ library in `test/gui/probe/`. It is linked into the `owncloud` executable only with
  `-DWITH_GUI_TEST_PROBE=ON`, and only activated by `OWNCLOUD_GUI_TEST_PROBE=<name>`. It serves
  newline-delimited JSON over a `QLocalServer` and resolves Squish-style selectors against QWidget, QModelIndex
  and Qt Quick trees.
- **Harness:** a Python package in `test/gui/harness/`. It provides the modules `squish`, `test`, `object`,
  `objectmaphelper` and `squishinfo`, plus a Gherkin runner with Squish semantics. It executes each
  `tst_*/test.py` as-is.

**Tech Stack:**
- Probe: C++17, Qt 6.8 (Core, Network, Widgets, Test, Quick, QuickWidgets), CMake.
- Harness: Python 3.10, `gherkin-official`, pytest.
- CI: GitHub Actions on `owncloudci/squish:fedora-42-8.1.0-qt68x-linux64`, with an `owncloud/ocis-rolling:20260610`
  service.

**Spec:** `docs/superpowers/specs/2026-11-24-squish-free-gui-tests-design.md`

## Global Constraints

- No edits to `.feature`, `shared/steps/*.py`, `shared/scripts/**`, `names.py`, `bdd_hooks.py` or
  `tst_*/test.py`. The only exception is `@skip @issue-<n>` tags in Task 13. Any other unavoidable edit is
  listed in the PR body with its reason.
- The probe is OFF by default (`option(WITH_GUI_TEST_PROBE ... OFF)`). It is inert without the
  `OWNCLOUD_GUI_TEST_PROBE` env var, listens only on `QLocalServer` (no TCP), and needs no code changes in
  `src/` beyond CMake wiring.
- All probe work runs on the GUI thread.
- Default timeout: `testSettings.waitForObjectTimeout = 20000` ms. A timeout raises `LookupError`.
- Linux is the only platform built and verified. No video recording: `test.startVideoCapture` and
  `test.stopVideoCapture` log and do nothing.
- Public repo: no Jira keys, CVE/GHSA IDs or "security"/"vulnerab…" wording in commits, the PR, branch
  names or the changelog. Commit trailer: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Python code passes `make -C test/gui python-lint` (black 24.3, pylint with `test/gui/.pylintrc`).
- Work happens on branch `gui-test-harness`. Subagents get their own worktree under
  `/Users/thomas.mueller/Development/squish/.worktrees/` (per `~/.claude/CLAUDE.md`).
- Before any push to a branch with an open PR, and before `gh pr create`, run the review gate in
  `~/.claude/CLAUDE.md` (code-review at `medium`, then `pre-submit-review.sh mark`).

## Review Focus

1. **An object dies between find and use**, e.g. a dialog closing after `waitForObject` returns. Expected: a
   `RuntimeError("object no longer exists")` in Python, and no crash or assert in the client. Covered by tests
   in Task 2 and Task 7.
2. **Selector value types are loose.** The suite writes `"column": "0"` next to `"column": 0`, and
   `"visible": 1` next to `True`. Expected: numbers, numeric strings and booleans compare equal by value.
   Covered by a test in Task 3.
3. **A stale socket from a crashed run.** Expected: the probe calls `QLocalServer::removeServer(name)`
   before `listen`, and the harness uses a fresh name per launch. Covered by a test in Task 2.
4. **The client crashes mid-scenario.** Expected: the scenario is reported as an error, `OnScenarioEnd`
   hooks still run (`bdd_hooks.py` kills leftovers and collects stack traces), and the next scenario starts
   cleanly. Covered by a test in Task 9.
5. **Non-ASCII text in `squish.type` and in property values**, such as umlauts or emoji in file names.
   Expected: exact UTF-8 round-trip. Covered by tests in Task 6 and Task 7.

---

## Wire protocol (shared by Tasks 2–8)

- Transport: one JSON object per line, UTF-8.
  - Request: `{"id": int, "method": str, "params": {...}}`.
  - Response: `{"id": int, "result": any}` or `{"id": int, "error": {"code": str, "message": str}}`.
- Error codes: `BadRequest`, `NoSuchObject` (unknown handle), `DeadObject`, `NoSuchProperty`,
  `MethodNotAllowed`, `NotFound`.
- **Encoding object references:**
  - A live object is `{"$handle": int, "$class": str}`. `$class` is the Squish type name: the `QMetaObject`
    class name, the QML name for Quick items, or `QModelIndex`/`TabItem`/`HeaderViewItem`.
  - A null object is `{"$handle": null}`.
- **Encoding selector values:**
  - Nested selector: a JSON object without `$`-keys.
  - Handle reference: `{"$handle": n}`.
  - Regular expression: `{"$regex": "..."}`.
  - Wildcard: `{"$wildcard": "..."}`.
- **Methods:**

  | method | params | result |
  |---|---|---|
  | `hello` | – | `{"pid": int, "qtVersion": str}` |
  | `find` | `selector`, `ready: bool` | handle or null object |
  | `findAll` | `selector` | `[handle]` |
  | `children` | `handle` | `[handle]` |
  | `get` | `handle`, `name` | value |
  | `call` | `handle`, `name`, `args: []` | value |
  | `mouseClick` | `handle`, `x?`, `y?`, `button`, `modifiers`, `clicks: 1\|2` | null |
  | `typeText` | `handle?`, `text` | null (no handle means the focus widget) |
  | `activateItem` | `handle` | null |
  | `clipboardText` | – | str |
  | `screenshot` | `path` | null |
  | `activeModalWidget` | – | handle or null object |
  | `release` | – | null (drops all handles) |

  `ready: true` additionally requires the object to be visible and enabled. For items it requires that the
  view is visible.

---

### Task 1: Restore both GUI test workflows (plain reverts)

**Files:**
- Restore: `.github/workflows/gui-tests.yml` (revert `5029d6f25`, #12618)
- Modify: `.github/workflows/branded-client.yml` (revert `3bd29f739`, #12620)

- [ ] **Step 1: Run** `git revert --no-edit 5029d6f25 && git revert --no-edit 3bd29f739`. Commit messages
  stay git's default `Revert "…"` text, with the trailer added via
  `git commit --amend -m "$(git log -1 --format=%B)" -m "Co-Authored-By: …"`.
- [ ] **Step 2: Verify.** Run `git diff HEAD~2 --stat`. Expected: `gui-tests.yml` with +152, and
  `branded-client.yml` with +2 -2 (the `schedule:` and `cron` lines uncommented).

### Task 2: Probe core: build option, activation, protocol, handles

**Files:**
- Modify: `CMakeLists.txt` (add `option(WITH_GUI_TEST_PROBE "Build the GUI test probe into the client (never for releases)" OFF)`
  next to the other options at lines 89–103)
- Modify: `src/gui/CMakeLists.txt:150` (`if(WITH_GUI_TEST_PROBE) target_link_libraries(owncloud PRIVATE guitestprobe) endif()`)
- Modify: top-level `CMakeLists.txt`, just before `add_subdirectory(src)` (line 141), adding
  `if(WITH_GUI_TEST_PROBE) add_subdirectory(test/gui/probe) endif()`. This way the target exists when
  `src/gui` links it.
- Create: `test/gui/probe/CMakeLists.txt`
  - `add_library(guitestprobe OBJECT …)`. An OBJECT library makes sure the startup function is not dropped
    by the linker.
  - Links Qt6::Widgets, Network, Test, Quick, QuickWidgets.
  - Tests are added with `ecm_add_test` / `add_test` when `BUILD_TESTING`.
- Create: `test/gui/probe/probe.{h,cpp}` (startup, server, dispatch)
- Create: `test/gui/probe/registry.{h,cpp}` (handles)
- Create: `test/gui/probe/jsonvalue.{h,cpp}` (QVariant ↔ JSON)
- Test: `test/gui/probe/tests/tst_protocol.cpp`

**Interfaces:**
- Produces, in C++ namespace `OCC::GuiTestProbe`:
  - `class Probe : public QObject` with:
    - `static Probe *startIfRequested()`, which reads `OWNCLOUD_GUI_TEST_PROBE` and returns nullptr if it
      is unset.
    - `bool listen(const QString &name)`.
    - `QJsonObject handle(const QJsonObject &request)`, so tests can drive it without a socket.
    - `void registerMethod(const QString &name, std::function<QJsonValue(const QJsonObject &params)>)`.
      Later tasks add their methods through this.
  - `class Registry` with:
    - `QJsonObject ref(QObject *)`.
    - `QJsonObject ref(const QModelIndex &)`.
    - `QJsonObject ref(const SyntheticItem &)`, where `SyntheticItem` is `{QPointer<QObject> owner; QString kind; int index;}`
      for `TabItem` and `HeaderViewItem`.
    - `Target resolve(const QJsonValue &)`, which throws `ProbeError{code, message}` for unknown or dead
      handles.
    - `void clear()`.
  - `Target` is a `std::variant<QObject*, QPersistentModelIndex, SyntheticItem>`.
  - `QJsonValue toJson(const QVariant &, Registry &)`. `QObject*` values become refs. Also handles
    QString, numbers, bool, QStringList, QVariantList/Map, QUrl, QDateTime (ISO), QSize/QRect (objects).
  - Startup: `Q_COREAPP_STARTUP_FUNCTION` calls `Probe::startIfRequested()` through a zero-timer, so it
    runs once the event loop starts.
  - Methods registered here: `hello`, `get`, `children` (QObject children only; items are added in
    Task 4), `release`.
  - Property access in `get` (QObject): `QMetaProperty` first, then dynamic properties, then `objectName`.
    Anything else is `NoSuchProperty`.

- [ ] **Step 1: Write failing tests** in `tst_protocol.cpp` (QtTest, run with `QT_QPA_PLATFORM=offscreen`):
  - `hello` returns `pid == QCoreApplication::applicationPid()`.
  - `get` on a `QLabel` with text `"Grüße 🙂"` returns exactly that string.
  - An unknown method returns error `BadRequest`.
  - Deleting a widget and then calling `get` on its handle returns `DeadObject`.
  - `children` returns refs whose `$class` is the child class names.
  - `release` makes the old handles return `NoSuchObject`.
  - After `QLocalServer::listen("x")` by a stale server that is left unclosed, `Probe::listen("x")` still
    succeeds.
  - In a build without the env var, `Probe::startIfRequested()` returns nullptr.
- [ ] **Step 2: Run** `cmake -S . -B build-probe -G Ninja -DWITH_GUI_TEST_PROBE=ON -DBUILD_TESTING=ON && ninja -C build-probe && ctest --test-dir build-probe -R tst_protocol --output-on-failure`.
  Expected: FAIL (methods missing).
- [ ] **Step 3: Implement** `Probe`, `Registry` and `toJson` as specified. Make one `QLocalSocket`
  connection at a time, buffer on `readyRead` and split on `\n`.
- [ ] **Step 4: Run** the same `ctest` command. Expected: PASS.
- [ ] **Step 5: Verify the default build is clean:**
  `cmake -S . -B build-default -G Ninja && ninja -C build-default owncloud && ! nm -C build-default/bin/owncloud | grep -q GuiTestProbe`.
  Expected: exit 0.
- [ ] **Step 6: Commit** with the message `test(gui): add GUI test probe core behind WITH_GUI_TEST_PROBE`.

### Task 3: Widget selector resolution

**Files:**
- Create: `test/gui/probe/selector.{h,cpp}`
- Modify: `test/gui/probe/probe.cpp` (register `find` and `findAll`)
- Test: `test/gui/probe/tests/tst_selector.cpp`

**Interfaces:**
- Consumes: `Registry`, `Target`, `ProbeError` (Task 2).
- Produces:
  - `class SelectorEngine` with:
    - `std::vector<Target> findAll(const QJsonObject &selector)`.
    - `std::optional<Target> find(const QJsonObject &selector, bool ready)`.
    - `bool matches(const Target &, const QJsonObject &selector)`.
    - `void addTypeResolver(std::function<std::optional<std::vector<Target>>(const QJsonObject &sel, const Target &scope)>)`.
      Tasks 4 and 5 use it to add item and QML search under a scope.
  - `bool valueMatches(const QJsonValue &want, const QVariant &have)`.
- **Search order:** depth-first, in `QObject::children()` order, starting from
  `QApplication::topLevelWidgets()` (or from the `container`/`window` scope when one is given). Results are
  ranked in that order, and `occurrence: n` (1-based) picks the n-th match.
- **Matching rules:**
  - `type`: equal to the Squish type name (Task 2 `$class`).
  - `name`: `objectName`.
  - `unnamed: 1`: `objectName` is empty.
  - `visible`: `isVisible()`.
  - `window`: the object's `window()` matches the nested selector.
  - `container`: some ancestor matches.
  - `windowTitle`, `text`, `checkable`, `orientation`, and any other key: a property with that name.
    Enums compare by key name or by integer.
  - `aboveWidget: S`: this widget's global bottom is ≤ S's global top, and the two overlap horizontally.
  - `leftWidget: S`: this widget's global left is ≥ S's global right, and the two overlap vertically.
    For both, the nearest candidate wins.
  - `valueMatches`: `$regex` is a `QRegularExpression` search, `$wildcard` is
    `QRegularExpression::wildcardToRegularExpression`, numbers and numeric strings and bools compare by value,
    and anything else is a string compare.

- [ ] **Step 1: Write failing tests** against an in-process widget tree:
  - A `QDialog` named `"Settings"` holds a `QStackedWidget` `"stack"`, which holds two unnamed `QLabel`s
    above and below a `QCheckBox`.
  - Each of these selectors finds exactly the expected object:
    - `{"name":"stack","type":"QStackedWidget","visible":1,"window":{"name":"Settings","type":"QDialog","visible":1}}`
    - `{"type":"QLabel","unnamed":1,"occurrence":2,"container":{...stack}}`
    - `{"text":{"$regex":"^Errors: [0-9]+$"},"type":"QLabel"}`
    - `{"type":"QLabel","aboveWidget":{checkbox},"unnamed":1}` (the upper label)
  - `valueMatches("0", 0)`, `valueMatches(1, true)` and `valueMatches(true, "true")` are all true.
  - `find(..., ready=true)` returns nothing for a hidden or disabled widget, while `ready=false` finds it.
  - A namespaced class (`class OCC::TestDialog : public QDialog` with `Q_OBJECT`) matches
    `{"type":"OCC::TestDialog"}`.
- [ ] **Step 2: Run** `ctest --test-dir build-probe -R tst_selector --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement** `SelectorEngine` and `valueMatches`, and register `find`/`findAll`.
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): resolve Squish-style widget selectors in the probe`.

### Task 4: Item views: QModelIndex, TabItem, HeaderViewItem

**Files:**
- Create: `test/gui/probe/items.{h,cpp}`
- Modify: `test/gui/probe/probe.cpp` (`children` and `get` for items)
- Test: `test/gui/probe/tests/tst_items.cpp`

**Interfaces:**
- Consumes: `SelectorEngine::addTypeResolver`, `Registry::ref` (Tasks 2–3).
- Produces: `void registerItemSupport(SelectorEngine &, Registry &)`, with these semantics:
  - `type: "QModelIndex"`:
    - The scope is the `container`. If the container is a view, search its model's top-level rows. If it
      is an index, search that index's children.
    - Keys: `row`, `column` (default: any), and `text` (`Qt::DisplayRole`).
    - Search is breadth-first over the scope's direct rows only. That is Squish's behaviour; the suite
      nests `container` for deeper levels.
  - `type: "TabItem"`: the scope is a `QTabBar`, or a `QTabWidget`'s `tabBar()`. Keys are `index` and
    `text`.
  - `type: "HeaderViewItem"`: the scope is a `QHeaderView`. Keys are `text` (`headerData`) and `section`.
  - **Item properties for `get`:**
    - QModelIndex: `text`, `row`, `column`, `checkState`, `selected`, `visible` (inside the viewport).
    - TabItem: `text`, `index`.
    - HeaderViewItem: `text`, `section`.
  - **`children`:** for a view, its top-level indexes (column 0). For an index, its child indexes.
  - `QRect itemRect(const Target &)`: the item's rectangle in its widget's coordinates. QModelIndex uses
    `view->visualRect` (viewport). TabItem uses `tabRect`. HeaderViewItem uses `sectionViewportPosition` and
    `sectionSize`. Task 6 uses it for clicks.

- [ ] **Step 1: Write failing tests:**
  - A `QTreeWidget` with `"Personal"` → `"folder1 (13 B)"`:
    - `{"type":"QModelIndex","container":tree,"column":"0","text":"Personal"}` resolves.
    - The nested `container` finds `folder1 (13 B)`.
    - `children(tree)[0].text == "Personal"`.
  - A `QTableWidget` with `row: 1, column: 6` returns that cell's `text`.
  - A `QTabWidget` with tabs `"Local Activity","Not Synced"`: `{"type":"TabItem","container":tabs,"index":1}`
    has text `"Not Synced"`.
  - A header with `"Name","Size"`: `{"type":"HeaderViewItem","container":header,"text":"Size"}` has
    `section == 1`.
- [ ] **Step 2: Run** `ctest --test-dir build-probe -R tst_items --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement** `registerItemSupport` and `itemRect`.
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): support model, tab and header items in the probe`.

### Task 5: Qt Quick items inside QQuickWidget

**Files:**
- Create: `test/gui/probe/quick.{h,cpp}`
- Test: `test/gui/probe/tests/tst_quick.cpp` (with `tst_quick.qml` in a test resource)

**Interfaces:**
- Consumes: `SelectorEngine::addTypeResolver`, `Registry` (Tasks 2–3).
- Produces:
  - `void registerQuickSupport(SelectorEngine &, Registry &)`.
  - `QString squishTypeName(const QObject *)`, which the Task 2 `$class` uses for Quick items.
- **Traversal:** below any `QQuickWidget`, children continue at `rootObject()` and recurse through
  `QQuickItem::childItems()`. Both `container` and the `children` method follow this.
- **`squishTypeName`:** take `metaObject()->className()`, strip a `_QMLTYPE_\d+` or `_QML_\d+` suffix, then
  strip a leading `QQuick`. So `QQuickTextField` becomes `TextField`, `QQuickLabel` becomes `Label`, and
  `SpaceDelegate_QMLTYPE_3` becomes `SpaceDelegate`.
  - The builder confirms this against Squish's QML type names for `Label`/`TextField`/`Button`/`ScrollView`,
    and also against `Text`, which is used in the suite. Run
    `grep -n '"type"' test/gui/shared/scripts/names.py test/gui/shared/scripts/pageObjects/*.py`.
- **`id`:** `qmlContext(obj)->nameForObject(obj)`, walking parent contexts.
- **Properties:** `get` on a Quick item works the same as on QObjects. `visible` is `QQuickItem::isVisible()`
  combined with the host widget's visibility.
- **`itemRect` for Quick items:** `mapRectToScene(boundingRect())`, in the `QQuickWidget`'s coordinates.

- [ ] **Step 1: Write failing tests:**
  - A `QQuickWidget` named `quickWidget` loads a QML file with `ScrollView { id: scrollView }`, holding
    `TextField { id: userNameField }`, `Button { id: loginButton; text: "Log in" }` and `Label { text: "alice" }`.
  - `{"container":{"name":"quickWidget","type":"QQuickWidget"},"id":"userNameField","type":"TextField","visible":true}`
    resolves.
  - `get(label,"text") == "alice"`.
  - The `$class` of the button is `"Button"`.
  - The `ScrollView` resolves through `{"id":"scrollView","type":"ScrollView","unnamed":1}`, the shape used at
    `names.py:50`.
- [ ] **Step 2: Run** `ctest --test-dir build-probe -R tst_quick --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): resolve Qt Quick items in the probe`.

### Task 6: Calls and input

**Files:**
- Create: `test/gui/probe/actions.{h,cpp}` (calls, allowlist, input)
- Create: `test/gui/probe/keys.{h,cpp}` (Squish key syntax)
- Test: `test/gui/probe/tests/tst_actions.cpp`

**Interfaces:**
- Consumes: Tasks 2–5, including `itemRect`.
- Produces: the methods `call`, `mouseClick`, `typeText`, `activateItem`, `clipboardText`, `screenshot` and
  `activeModalWidget`.
- **`call`:**
  - Slots and `Q_INVOKABLE`s go through `QMetaMethod::invoke`, matching on name and argument count.
  - Explicit allowlist: `model()` on `QAbstractItemView`; `rowCount()`, `columnCount()`, `index(r,c)` and
    `data(idx, role=DisplayRole)` on `QAbstractItemModel`.
  - Anything else is `MethodNotAllowed`, naming the class and the method.
- **`mouseClick`:**
  - Target point: `(x, y)` relative to the widget or item, or its centre if omitted.
  - For QModelIndex items the event goes to `view->viewport()`. For Quick items it goes to the hosting
    `QQuickWidget`.
  - Uses `QTest::mouseClick`, or `mouseDClick` for `clicks: 2`. `button` and `modifiers` are Qt integer values.
- **`typeText`:** the target gets focus first. Text is split into literal runs and `<Key>` tokens.
- **`keys`:** `std::vector<KeyStroke> parseSquishKeys(const QString &)`, where `KeyStroke` is
  `{Qt::Key key; Qt::KeyboardModifiers mods; QString text;}`.
  - Tokens: `<Return>`, `<Enter>`, `<Tab>`, `<Backspace>`, `<Delete>`, `<Escape>`, `<Esc>`, `<Space>`,
    `<Up>`/`<Down>`/`<Left>`/`<Right>`, `<Home>`, `<End>`, `<F1>`…`<F12>`.
  - Modifier combinations: `<Ctrl+A>`, `<Shift+Tab>`, `<Alt+F4>`.
  - Matching is case-insensitive. An unknown token is `BadRequest`. `<<` is a literal `<`.
- **`activateItem`:**
  - A `QAction` is triggered with `trigger()`.
  - A `QModelIndex` is activated by `setCurrentIndex` plus the `activated(index)` signal.
  - For a menu `QAction` that belongs to an open `QMenu`, the item is clicked instead.
- **`clipboardText`:** `QGuiApplication::clipboard()->text()`.
- **`screenshot`:** `QGuiApplication::primaryScreen()->grabWindow(0).save(path)`.
- **`activeModalWidget`:** a ref or the null object.

- [ ] **Step 1: Write failing tests:**
  - `parseSquishKeys("ab<Ctrl+A><Return>Ä")` gives `[a, b, Ctrl+A, Return, Ä]`.
  - `typeText(lineEdit, "Grüße 🙂")` leaves the edit's text equal to that string.
  - `typeText(lineEdit, "x<Ctrl+A><Backspace>")` leaves it empty.
  - `mouseClick` on a `QPushButton` emits `clicked` once. `clicks: 2` on a tree index emits `doubleClicked`.
  - `mouseClick(index, 11, 11, …)` on a checkable `QTreeWidgetItem` toggles its check state. That is the
    suite's "checkbox at (11,11)" pattern.
  - `call(tree, "model")` then `call(model, "rowCount")` returns the row count.
  - `call(widget, "deleteLater")` succeeds (a slot). `call(widget, "setParent", [null])` is `MethodNotAllowed`.
  - `activateItem(action)` emits `triggered`.
  - With a modal `QMessageBox` open, `activeModalWidget` returns its ref.
- [ ] **Step 2: Run** `ctest --test-dir build-probe -R tst_actions --output-on-failure`. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): add calls and input synthesis to the probe`.

### Task 7: Python client, proxies and the `squish`, `object`, `objectmaphelper` modules

**Files:**
- Create: `test/gui/harness/harness/__init__.py`
- Create: `test/gui/harness/harness/client.py`
- Create: `test/gui/harness/harness/proxy.py`
- Create: `test/gui/harness/harness/selectors.py`
- Create: `test/gui/harness/shim/squish.py`, `shim/object.py`, `shim/objectmaphelper.py`. The `shim/`
  directory is put first on `sys.path` by the runner.
- Create: `test/gui/harness/tests/fake_probe.py` (an in-process fake that speaks the wire protocol over a
  socketpair and holds a dict-based object tree)
- Test: `test/gui/harness/tests/test_squish_api.py`
- Modify: `test/gui/requirements.txt`. Add `gherkin-official==29.*` and `pytest==8.*`, both with
  `python_version >= "3.10"`.

**Interfaces:**
- **`harness.client.ProbeClient(sock)`:**
  - `.request(method: str, **params) -> Any`.
  - Raises `ProbeError(code, message)`. A `DeadObject` error becomes `RuntimeError("object no longer exists")`
    at the API layer.
- **`harness.proxy.RemoteObject`:**
  - `__getattr__(name)` does a `get`. On `NoSuchProperty` it returns a callable that does `call`.
  - `__getitem__(name)` is the same as `get`, for the suite's `obj["section"]` pattern.
  - `hasattr` is False on `NoSuchProperty` when the name is also not a method.
  - `__str__` gives `"<null>"` for the null object.
  - `__eq__`/`__hash__` go by handle. `__bool__` is False for the null object.
  - Returned refs are wrapped recursively.
- **`harness.selectors.encode(sel) -> dict`:**
  - `RegularExpression` becomes `$regex`, `Wildcard` becomes `$wildcard`.
  - A `RemoteObject` becomes `$handle`.
  - Nested dicts recurse.
- **The `squish` module:**
  - `waitForObject(sel, timeout=None)` polls `find` with `ready=True`.
  - `waitForObjectExists(sel, timeout=None)` polls with `ready=False`.
  - `waitForObjectItem(container, itemText, timeout=None)` is sugar for
    `{"container": c, "text": itemText, "type": "QModelIndex"}`. The builder checks its exact use sites
    first.
  - The three wait functions poll every 100 ms. Raw handles pass through.
  - `findObject(sel)` uses `ready=False` and raises `LookupError` immediately.
  - `findAllObjects(sel)`.
  - `clickButton(obj)` and `mouseClick(obj, x=None, y=None, modifiers=0, button=Qt.LeftButton)`.
    The suite passes `(obj, x, y, Qt.NoModifier, Qt.LeftButton)`: the order is modifiers, then button.
  - `doubleClick(obj, x=None, y=None, modifiers=0, button=Qt.LeftButton)`.
  - `type(obj, text)` and `nativeType(text)`. `nativeType` sends to the focus widget.
  - `activateItem(obj)`.
  - `getClipboardText()` and `saveDesktopScreenshot(path)`.
  - `snooze(seconds)`.
  - `waitFor(cond, timeout_ms=None)`. `cond` is a callable or a string that is evaluated in the caller's
    frame. It returns bool and never raises on timeout.
  - `Qt` is an object with `NoModifier=0`, `LeftButton=1`, `RightButton=2`, `ShiftModifier=0x02000000`,
    `ControlModifier=0x04000000`, `AltModifier=0x08000000`.
  - `startApplication`, `applicationContextList` and `currentApplicationContext` are re-exported from
    Task 8's `harness.aut`.
  - Module global `QApplication.activeModalWidget()`.
  - `object.children(obj) -> list[RemoteObject]`. `object.exists(sel) -> bool` is a find with no wait.
  - `objectmaphelper`: `RegularExpression(pattern)` and `Wildcard(pattern)`, both with `.pattern`, and
    `__all__` exporting both.

- [ ] **Step 1: Write failing tests** against `fake_probe`:
  - `waitForObject` returns after the fake object becomes ready on the third poll.
  - It raises `LookupError` after `testSettings.waitForObjectTimeout = 300` with no object.
  - `waitForObjectExists` accepts a hidden object.
  - `encode({"text": RegularExpression("^a$"), "container": proxy})` gives
    `{"text":{"$regex":"^a$"},"container":{"$handle":7}}`.
  - `proxy.text == "Grüße 🙂"`.
  - `proxy.model().rowCount() == 3`.
  - `proxy["section"] == 1`.
  - `str(null_proxy) == "<null>"`.
  - `hasattr(proxy, "nope") is False`.
  - A dead handle raises `RuntimeError`.
  - `mouseClick(o, 11, 11, Qt.NoModifier, Qt.LeftButton)` sends
    `{"x":11,"y":11,"modifiers":0,"button":1,"clicks":1}`.
- [ ] **Step 2: Run** `cd test/gui/harness && python3.10 -m pytest tests/test_squish_api.py -q`. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): add Squish-compatible Python API on top of the probe`.

### Task 8: AUT launch, application contexts, `test`, `squishinfo`, `testSettings`

**Files:**
- Create: `test/gui/harness/harness/aut.py`
- Create: `test/gui/harness/harness/results.py`
- Create: `test/gui/harness/shim/test.py`, `shim/squishinfo.py`
- Test: `test/gui/harness/tests/test_aut_results.py` (uses a tiny Python script that serves `fake_probe`
  over a `QLocalServer`-compatible Unix socket at the name it is given)

**Interfaces:**
- **`harness.aut.configure(aut_paths: dict[str, str], env: dict[str, str])`** is called by the runner.
- **`startApplication(cmdline: str) -> ApplicationContext`:**
  - Resolve the first word against `aut_paths`, which map AUT name to directory, the same as `server.ini`.
  - Set `OWNCLOUD_GUI_TEST_PROBE=oc-guitest-<pid>-<n>`, plus `env` and the suite `ENVVARS`.
  - `subprocess.Popen` the command, then connect to the socket. The Linux path is
    `/tmp/oc-guitest-…` (`QLocalServer` default). Retry until `AUTPMTimeout=15000` ms, then raise
    `RuntimeError` with the process's exit code and the stderr tail.
  - Becomes the current context.
- **`ApplicationContext`:** `.pid`, `.name`, `.isRunning`, `.detach()` (closes the socket and terminates
  the process if still running), and `.client`.
- `applicationContextList() -> list[ApplicationContext]` and `currentApplicationContext()`.
- **The `test` module:**
  - `compare(a, b, msg="")`, `verify(cond, msg="")`, `fail(msg, detail="")`, `warning(msg)`, `log(msg)`.
  - `resultCount(kind: "passes"|"fails"|"errors"|"warnings") -> int`, cumulative across the whole run, as
    `bdd_hooks.py:147` relies on.
  - `startVideoCapture()` and `stopVideoCapture()` are no-ops that log.
  - The module also exposes `harness.results.Results`, the per-scenario record that reports read.
- **`testSettings`** is a module global in `squish` and in builtins:
  - `throwOnFailure: bool = False`. When True, `compare`/`verify`/`fail` raise `AssertionError` after
    recording.
  - `waitForObjectTimeout: int = 20000`.
- **`squishinfo`:** `testCase` is the absolute path of the current `tst_*` directory. `resultDir` is the
  report directory or `""`. Both are set by the runner.

- [ ] **Step 1: Write failing tests:**
  - `startApplication("fakeaut -s --logfile x")` resolves through `aut_paths`, passes the args, and connects.
    The context `pid` equals the probe's `hello.pid`.
  - A command that exits immediately raises `RuntimeError` containing the exit code, within the timeout.
  - `compare(1, 2)` raises with `throwOnFailure=True`, and either way increments `resultCount("fails")`.
  - `applicationContextList()` lists the started context. After `detach()`, `isRunning` is False.
- [ ] **Step 2: Run** `python3.10 -m pytest tests/test_aut_results.py -q`. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): launch the client and record results without Squish`.

### Task 9: Suite loading and the Gherkin runner with Squish semantics

**Files:**
- Create: `test/gui/harness/harness/suite.py` (reads `suite.conf` and `envs.txt`, executes `test.py`)
- Create: `test/gui/harness/harness/bdd.py` (step registry, hooks, context, scenario execution)
- Create: `test/gui/harness/harness/tags.py`
- Test: `test/gui/harness/tests/test_bdd.py`, with `tests/data/suite_min/` (a minimal suite: `suite.conf`,
  `shared/steps/s.py`, `shared/scripts/bdd_hooks.py`, `tst_a/test.feature`, `tst_a/test.py`, all copying
  the real suite's shapes)

**Interfaces:**
- **`harness.tags.TagFilter(exprs: list[str])`:** `.accepts(tags: set[str]) -> bool`.
  - Each `--tags` value is one clause, and clauses are ANDed.
  - Within a clause, comma means OR, and a leading `~` negates. `~@skip` means "not @skip".
  - Tags are inherited from Feature to Scenario to Examples.
- **`harness.bdd.StepRegistry`:**
  - `.add(pattern: str, fn, regexp: bool)` and `.match(text: str) -> (fn, args)`. `match` raises
    `UndefinedStep` or `AmbiguousStep`.
  - Placeholders: `|any|` is `(.+?)`, `|word|` is `(\w+)`, `|integer|` is `([-+]?\d+)` with the arg passed
    as int. The rest of the pattern is `re.escape`d and anchored `^…$`.
  - With `regexp=True` the pattern is used as given, anchored.
  - Matching ignores the keyword. Given, When, Then, And and But all match any decorator.
- **Decorators** `Given`, `When`, `Then` and `Step` are injected into builtins, each as
  `(pattern, regexp=False)`.
- **Hooks:** `OnFeatureStart`, `OnFeatureEnd`, `OnScenarioStart`, `OnScenarioEnd`, `OnStepStart` and
  `OnStepEnd` are decorators taking `fn(context)`. They are injected into builtins.
- **`Context`:**
  - `.title`: the scenario or feature name.
  - `.table`: a list of rows, each a list of str, including the header.
  - `.multiLineText`: the doc string split on `\n`.
  - `.userData`: None by default and preserved across steps of one scenario.
- **Squish entry points** injected into builtins when executing `test.py`:
  - `source(path)` runs a file in the caller's globals. For the virtual `bdd.py` it does nothing.
  - `findFile(kind, name)`: `'scripts'` searches `<suite>/shared/scripts` and `<harness>/shim`, and
    `'python/bdd.py'` returns a virtual path.
  - `setupHooks(path)` and `collectStepDefinitions(*dirs)` resolve relative to the `tst_*` directory.
  - `runFeatureFile(path)`.
- **`harness.suite.run_suite(suite_dir, testcases, tag_filter, abort_on_fail, reporter) -> int`:** returns
  the exit code, `0` only if nothing failed or errored.
  - `sys.path` gets `shim/` and `<suite>/shared/scripts` first.
  - `os.environ` gets `ENVVARS`.
  - The working directory is the `tst_*` directory while it runs.
- **Scenario execution:**
  - Background steps, then scenario steps. Scenario Outline Examples are expanded, with `<x>` substituted.
  - Once a step fails, the remaining steps are skipped.
  - An exception in a step is an error. A `test.*` failure is a fail.
  - `OnScenarioEnd` always runs, even when the AUT crashed.
  - `--abortOnFail` stops the run after the first failed scenario.

- [ ] **Step 1: Write failing tests:**
  - `TagFilter(["~@skip","~@skipOnLinux"])` rejects `{"@skip"}` and accepts `{"@smokeTest"}`.
    `TagFilter(["@a,@b"])` accepts `{"@b"}`.
  - `'user "|any|" creates a file "|any|"'` matches `user "Alice" creates a file "a b.txt"` with args
    `("Alice","a b.txt")`.
  - `r'the user waits for (file|folder) "([^"]*)" to be synced'` with `regexp=True` matches.
  - A `@When` step matches a feature line `And …`.
  - Running `suite_min`:
    - The hooks fire in the order FeatureStart, ScenarioStart, steps, ScenarioEnd, FeatureEnd.
    - `context.table[0]` is the header row, and `'\n'.join(context.multiLineText)` equals the doc string.
  - When a step raises (the stand-in for an AUT crash), the scenario is an error, `OnScenarioEnd` still runs,
    and the next scenario still runs.
  - An undefined step fails with a message that contains the step text.
- [ ] **Step 2: Run** `python3.10 -m pytest tests/test_bdd.py -q`. Expected: FAIL.
- [ ] **Step 3: Implement.** Parse with `gherkin.parser.Parser` and `gherkin.pickles.compiler.Compiler`.
  Pickles already expand Background and Outlines, and give doc strings and tables.
- [ ] **Step 4: Run** the same command. Expected: PASS.
- [ ] **Step 5: Commit** with the message `test(gui): run Gherkin features with Squish step semantics`.

### Task 10: Reports and the runner CLI

**Files:**
- Create: `test/gui/harness/harness/report.py`
- Create: `test/gui/harness/harness/__main__.py`
- Test: `test/gui/harness/tests/test_cli_report.py`

**Interfaces:**
- **CLI:** `python3.10 -m harness` (run with `PYTHONPATH=test/gui/harness`). Flags:
  - `--testsuite DIR`, required.
  - `--testcase NAME`, repeatable. By default all `tst_*` run.
  - `--tags EXPR`, repeatable.
  - `--abortOnFail`.
  - `--envvar K=V`, repeatable. The value may contain `;` and `=` after the first `=`.
  - `--aut NAME=DIR`, repeatable. It replaces `server.ini`.
  - `--reportgen FORMAT,PATH`, repeatable. `html,<dir>` writes `<dir>/index.html`, and `junitxml,<file>`
    writes JUnit XML.
  - Exit code: the value from `run_suite`.
- **`Reporter`:** `feature_start`, `scenario_start`, `step(result, text, message, duration)`, `scenario_end`,
  `attach(path)` and `finish()`.
  - Screenshots that hooks save under `GUI_TEST_REPORT_DIR/screenshots` are linked from the failing
    scenario.
  - The HTML has one section per feature, one row per scenario with its status, and the failing step's
    message.

- [ ] **Step 1: Write failing tests:**
  - Running the CLI on `suite_min` with one failing scenario exits with 1.
  - The JUnit file has one `<testcase>` per scenario, and the failing one has a `<failure>` with the step
    message.
  - `index.html` contains every scenario title.
  - `--envvar "QT_LOGGING_RULES=sync.httplogger=true;gui.socketapi=false"` reaches the AUT env unchanged.
- [ ] **Step 2: Run** `python3.10 -m pytest tests/test_cli_report.py -q`. Expected: FAIL.
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run** `python3.10 -m pytest tests -q && make -C .. python-lint`. Expected: all pass, lint clean.
- [ ] **Step 5: Commit** with the message `test(gui): add runner CLI with HTML and JUnit reports`.

### Task 11: CI wiring and documentation

**Files:**
- Create: `test/gui/ci/run-gui-tests.sh`
- Modify: `.github/workflows/gui-tests.yml`, `.github/workflows/branded-client.yml`
- Modify: `test/gui/Makefile` (add target `harness-test: cd harness && python3.10 -m pytest tests -q`)
- Create or modify: `test/gui/README.md` (a section on running locally)
- Delete: `test/gui/ci/server.ini` and `test/gui/ci/client_server.ini`. Their AUT mappings become `--aut`
  flags.

**Interfaces:**
- **`run-gui-tests.sh`** reads `GUI_TEST_PARAMETERS` (the same tokens as the old `SQUISH_PARAMETERS`) and
  `AUT_NAME`/`AUT_DIR`. It:
  - Starts `Xvfb :1 -screen 0 1920x1080x24` with `DISPLAY=:1`, or the image's `/dockerstartup/vnc_startup.sh`
    if Xvfb is missing.
  - Starts `dbus-run-session`.
  - Unlocks gnome-keyring with `echo -n "$VNC_PW" | gnome-keyring-daemon -r --unlock`, reusing
    `wait_for_keyring` from the image's `entrypoint.sh`.
  - Sets `ulimit -c unlimited`.
  - Execs `python3.10 -m harness --aut "$AUT_NAME=$AUT_DIR" $GUI_TEST_PARAMETERS`.
  - The builder reads `/dockerstartup/vnc_startup.sh` and `entrypoint.sh` in the image
    (`owncloud-ci/squish`, `fedora/src/startup/`) and keeps whatever the client needs from them (D-Bus env,
    keyring, `HOME`).
- **Both workflows:**
  - cmake gets `-DWITH_GUI_TEST_PROBE=ON -DBUILD_TESTING=ON`, and the build adds `ctest -R tst_ --output-on-failure`
    for the probe tests. `BUILD_TESTING` was `OFF` before. If `ON` pulls in the full unit-test build, use
    `-DBUILD_TESTING=OFF` and build only `guitestprobe` tests with `-DWITH_GUI_TEST_PROBE_TESTS=ON`.
    Decide after timing it.
  - Remove `LICENSEKEY` and `SERVER_INI`. Rename `SQUISH_PARAMETERS` to `GUI_TEST_PARAMETERS`, with the same
    tags and the same `full-ci` expression in `gui-tests.yml`.
  - `run:` becomes `bash test/gui/ci/run-gui-tests.sh`.
  - `AUT_NAME` is `owncloud` in `gui-tests.yml` and `${{ secrets.CLIENT_NAME }}` lower-cased in the branded
    workflow. The old INI used `kiteworks`; derive it in the script with `${CLIENT_NAME,,}`.
  - `AUT_DIR` is `$GITHUB_WORKSPACE/build/bin`.
- **`gui-tests.yml` only:**
  - Add a top-level `permissions: contents: read`.
  - Every `actions/checkout` gets `persist-credentials: false`.
  - Set the pins equal to `branded-client.yml`: checkout `3d3c42e5…` v7.0.1, upload-artifact `cf430e03…`
    v7.0.2. Check that `setup-node` is current with `gh api repos/actions/setup-node/releases/latest`.
  - `lint-gui-test` gains a step `make -C test/gui harness-test`.

- [ ] **Step 1: Verify locally**, running from the repository root:
  - `bash -n test/gui/ci/run-gui-tests.sh`.
  - If available, `actionlint .github/workflows/gui-tests.yml .github/workflows/branded-client.yml`.
  - `zizmor .github/workflows/gui-tests.yml`, the same check the repo's `zizmor.yml` runs.

  Expected: no errors.
- [ ] **Step 2: Verify that no Squish references remain:**
  `grep -rnE 'LICENSEKEY|squishrunner|entrypoint.sh|SQUISH_PARAMETERS|server.ini' .github test/gui --exclude-dir=harness`.
  Expected: no output.
- [ ] **Step 3: Write the README section:** build with `-DWITH_GUI_TEST_PROBE=ON`, then
  `pip install -r test/gui/requirements.txt`, then
  `PYTHONPATH=test/gui/harness python3.10 -m harness --testsuite test/gui --aut owncloud=build/bin --tags ~@skip --testcase tst_syncing`.
  Include the required env vars, which match the `Run GUI test` step's env.
- [ ] **Step 4: Commit** with the message `ci: run GUI tests with the in-repo harness instead of Squish`.

### Task 12: Prove parity at the baseline `fb1c873a7d`

**Files:** none on `gui-test-harness`. This task only produces harness fixes, as commits.

- [ ] **Step 1: Ask the user before pushing anything to `owncloud/client`.** Pushing is outward-facing.
  Then create the throwaway branch `gui-test-harness-baseline` from `fb1c873a7d`, and run
  `git checkout gui-test-harness -- test/gui/probe test/gui/harness test/gui/ci/run-gui-tests.sh test/gui/requirements.txt .github/workflows/gui-tests.yml test/gui/Makefile`.
  Then apply the CMake wiring from Task 2 by hand, because the context may have drifted.
- [ ] **Step 2: Push** the branch to `owncloud/client`. `gui-tests.yml` runs on `push` only for master, so
  start it with `gh workflow run gui-tests.yml --ref gui-test-harness-baseline`. If it lacks
  `workflow_dispatch`, add that trigger on this throwaway branch only.
- [ ] **Step 3: Iterate.** Every failing scenario is a harness bug, because Squish passed this exact commit.
  Fix it on `gui-test-harness` with a probe or harness unit test that reproduces it, then cherry-pick to the
  baseline branch and re-run.
  - Expected: a green run with `--tags ~@skip --tags ~@skipOnLinux --abortOnFail`.
  - Then a second run without `--abortOnFail`, which must also be green.
- [ ] **Step 4: Clean up.** Delete the remote baseline branch once it is green, after asking the user.

### Task 13: Master results, skip tags, changelog, PR

**Files:**
- Modify: `test/gui/tst_*/test.feature` (tags only, and only for client regressions)
- Create: `changelog/unreleased/<PR number>.md`

- [ ] **Step 1: Run on the branch.** Push `gui-test-harness` (after asking the user) and run `gui-tests.yml`
  without `--abortOnFail` by putting `full-ci` in the PR title, or by dispatching. Also run
  `gh workflow run branded-client.yml --ref gui-test-harness`.
- [ ] **Step 2: Handle each failing scenario** (it passed at the baseline, so it is a client regression):
  - Draft a public GitHub issue naming the scenario, `passed at fb1c873a7d`, the failing step and its
    message. No internal ticket references.
  - Show the drafts to the user before creating them.
  - Then add `@skip @issue-<n>` to that scenario, and only that scenario.
- [ ] **Step 3: Write the changelog** `changelog/unreleased/<PR number>.md`:

  ```
  Change: Run GUI tests without Squish

  The GUI acceptance tests now run with an in-repository test harness instead of Squish, so they work
  on any machine and in CI without a license.

  https://github.com/owncloud/client/pull/<PR number>
  ```

- [ ] **Step 4: Before submitting:**
  - Run the public-repo grep from `~/.claude/CLAUDE.md`:
    `grep -iE 'CVE-|GHSA-|vulnerab|security|exploit|scan'` over the title, body, branch name, commit
    messages (`git log master..`) and changelog.
  - Run the review gate: code-review `$(git merge-base master HEAD)..HEAD medium`, fix, then mark.
- [ ] **Step 5: Open the PR** with `gh pr create`. The body includes:
  - what changed and why;
  - the baseline proof (run link);
  - the list of added skip tags with issue links;
  - any non-tag test edits with reasons (expected: none);
  - the attribution line `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.
