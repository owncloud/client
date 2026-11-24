# Running the GUI tests without Squish

Status: draft for review · 2026-11-24 · branch `gui-test-harness`

## Why

The GUI acceptance tests in `test/gui` depend on froglogic/Qt Squish. Squish costs money. Its license
server is not safe to expose to the internet, so only some people and runners can use it. Nightly GUI
tests have been off since #12620 ("Disable gui tests until we have valid squish license").

We want the existing suite to run on GitHub Actions, and on any developer machine, with no license and
no license server, and with no rewrite of the tests.

## Goals

- **Reuse the suite unchanged.** The `.feature` files, `shared/steps/*.py`, `shared/scripts/pageObjects/*.py`,
  `shared/scripts/helpers/*.py`, `names.py` and `bdd_hooks.py` run with **no edits**. Each `tst_*/test.py`
  runs unchanged too.
  - An edit is acceptable only where Squish behaviour cannot be reproduced. Each such edit is listed in the
    PR body with the reason.
- **Bring back both GUI test workflows, with Squish taken out:**
  - `GUI Tests` (`gui-tests.yml`): revert #12618, which deleted it. It runs on every pull request, on
    pushes to `master` and on tags, and nightly, against a local oCIS service container, without secrets.
    This is the main signal.
  - `GUI Tests (Branded Client)`: revert #12620, so it runs nightly again.
- **Everything is open source**, in `owncloud/client`, delivered as one public PR.

## Non-goals

- Running the GUI tests on Windows or macOS in CI. The design must not block that (the probe is pure Qt and
  uses `QLocalServer`), but only Linux is built and verified now, as it is today.
- Squish IDE features: recorder, object spy, verification point files. `shared/verificationPoints` is not
  referenced by any test.
- Video recording. CI runs with `RECORD_VIDEO_ON_FAILURE: false`, so `test.startVideoCapture` and
  `test.stopVideoCapture` become logged no-ops.
- Changing the `owncloudci/squish` image in `owncloud-ci/squish`. A Squish-free image is a follow-up in that
  repo.

## Anti-goals (a failure even if the tests pass)

- **The probe in a shipped build.** It must never be compiled into a shipped package. It is off unless a
  CMake option turns it on, and even when compiled in it stays inert unless an environment variable
  activates it.
- **A network-reachable listener.** The probe listens only on a local socket or named pipe. There is no TCP
  port.
- **Unmanageable divergence.** Rewriting `names.py` or the page objects into a new selector language,
  or anything else that makes the suite diverge from what Squish could still run during the transition.

## Constraints found in the repo

- Squish surface actually used, counted at `2d8be77`:
  - `squish`: `waitForObject` (74), `waitForObjectExists` (37), `clickButton` (30), `mouseClick` (27,
    including `(obj, x, y, Qt.NoModifier, Qt.LeftButton)`), `waitFor` (12), `snooze` (9), `type` (5),
    `waitForObjectItem` (4), `activateItem` (4), `doubleClick`, `nativeType`, `getClipboardText`,
    `startApplication`, `applicationContextList`, `saveDesktopScreenshot`, `click_tab`, `squish.Qt.*`.
  - `test`: `compare`, `log`, `fail`, `resultCount("fails"|"errors")`, `startVideoCapture` and
    `stopVideoCapture`, `testSettings.throwOnFailure`.
  - `object.children`, `object.exists`.
  - `from objectmaphelper import *`, which provides `RegularExpression` and `Wildcard`.
  - `QApplication.activeModalWidget()`, `currentApplicationContext()`.
  - BDD: `@Given/@When/@Then/@Step`, used as globals without import, with `|any|`, `|word|` and
    `regexp=True`; `@OnFeatureStart`, `@OnScenarioStart`, `@OnScenarioEnd`; `context.table`,
    `context.multiLineText`, `context.title`.
  - `test.py`: `source(findFile('scripts', 'python/bdd.py'))`, `setupHooks`, `collectStepDefinitions`,
    `runFeatureFile`.
- **Qt Quick is part of the tested UI.** The account wizard (`AccountConnectionWizard.py:47-55`), the login
  dialog (`EnterPassword.py`) and the spaces picker (`SyncConnectionWizard.py:78`, `names.py:49-80`) select
  QML items by short type and QML `id` (`TextField`, `Button`, `Label`, `ScrollView`). They sit inside a
  `QQuickWidget` or `OCC::QmlUtils::OCQuickWidget` container. Almost every scenario passes through the
  wizard. The QML sources are in `src/gui/**/qml`.
- **Additional surface**, found during review; implement everything the suite uses, not only this list:
  - Selector keys `occurrence`, `windowTitle`, `checkable`, `orientation`, `index`, `id`.
  - Squish's synthetic item types `TabItem` (`Activity.py:77`) and `HeaderViewItem`.
  - The `squishinfo` module (`resultDir`, `testCase`, in `ReportHelper.py`).
- Selectors are Python dicts over Qt object properties:
  - Keys: `type` (a Qt class name, including `OCC::` namespaces), `name` (`objectName`), `visible`, `window`,
    `container`, `text`, `unnamed`, `aboveWidget`, `leftWidget`.
  - `type: "QModelIndex"` items carry `container` (a view or another index), `column`, `row` and `text`.
  - Values may be `RegularExpression`.
- Tests read live properties (`.text`, `.checked`, `.enabled`, `.visible`) and call methods on proxies
  (`tree.model().rowCount()`). `str(proxy)` of a null object is `"<null>"`.
- `suite.conf`: `AUT=owncloud --showsettings`, `ENVVARS=envs.txt`, `IMPLICITAUTSTART=false`, `LANGUAGE=Python`,
  `OBJECTMAPSTYLE=script`, `WRAPPERS=Qt`. The AUT is started from `SetupClientHelper.start_client()` as
  `<client_name lower> -s --logfile … --logdebug --logflush`. CI maps `AUT/kiteworks` to `build/bin` via
  `ci/client_server.ini`.
- CI before #12618 (`.github/workflows/gui-tests.yml`, deleted in `5029d6f`):
  - `lint-gui-test` runs on pull requests only: `make -C test/gui python-lint` and `gherkin-lint`.
  - `gui-tests` runs in the same `owncloudci/squish` image with an `owncloud/ocis-rolling:20260610` service
    at `https://ocis:9200`. It builds the unbranded client (Debug), uses `SERVER_INI: test/gui/ci/server.ini`,
    and passes `--tags ~@skip --tags ~@skipOnLinux`.
    - `--abortOnFail` applies unless the PR title contains `full-ci`.
  - The only secret it used is `SQUISH_LICENSEKEY`.
  - Its action pins are older than the repo's current ones (`checkout` v6.0.2), and it has no `permissions:`
    block and no `persist-credentials: false`, unlike `branded-client.yml`.
- CI today (`.github/workflows/branded-client.yml`):
  - Runs in `owncloudci/squish:fedora-42-8.1.0-qt68x-linux64`, which provides the ownbuild/Craft Qt 6.8
    dependencies, VNC/X, D-Bus and gnome-keyring.
  - Builds the branded client using the `KW_THEME_PAT` secret.
  - Runs against a remote backend given by secrets, with predefined users.
  - The image's `entrypoint.sh` installs Squish with `LICENSEKEY` and runs `squishrunner`.
  - Triggers are `workflow_dispatch` plus a nightly cron, which #12620 commented out.
  - Repository secrets are only available to branches in `owncloud/client`, not to forks.

## Approaches considered

1. **An in-house Squish-compatible harness (chosen).** A probe compiled into the client exposes the Qt
   object tree over a local socket. A Python package provides modules named `squish`, `test`, `object` and
   `objectmaphelper`, plus a Gherkin runner with Squish semantics.
   - Pro: the suite runs as-is, and we have no third-party dependency.
   - Con: we maintain it. The surface is small and bounded (above).
2. **Spix as the probe, with our Python compatibility layer.** Spix is active and has recent, Qt 6-only
   QWidgets support. Its selectors are `objectName` slash-paths, and its XML-RPC returns plain values, not
   object handles.
   - Matching `window`, `container`, `QModelIndex` items, geometric keys and chained calls means heavy C++
     extension.
   - That is the same work as option 1, plus a dependency on a third party.
3. **Accessibility drivers** (AT-SPI WebDriver, Appium mac2, UIA). No change to the client, but selectors
   become accessible name and role, so `names.py` and all page objects would be rewritten. There is also no
   model access and a different backend per OS. This breaks the reuse goal.
   - Also rejected: qtwebdriver (inactive since 2022, Qt 5), GammaRay (an inspector without a stable probe
     API), pytest-qt (it only drives Python applications in-process).

The user picked **option 1**.

## Design

### 1. Probe (C++, in the client)

- **Location:** `test/gui/probe/`, built only when `-DWITH_GUI_TEST_PROBE=ON` (default `OFF`). Release
  and packaging workflows never set it.
- **Linking:** a library linked into the client executable. It starts itself through
  `Q_COREAPP_STARTUP_FUNCTION`, so `src/` needs no code changes beyond the CMake wiring. The builder makes
  sure the linker doesn't drop the startup object, by using an object library or whole-archive.
- **Activation:** inert unless the environment variable `QT_GUI_TEST_PROBE=<name>` is set. Then it
  opens a `QLocalServer` with that name, which is a Unix socket on Linux and macOS and a named pipe on
  Windows. It accepts one client and runs everything on the GUI thread.
- **Protocol:** newline-delimited JSON, with requests `{id, method, params}` and responses
  `{id, result | error}`. Methods:
  - `find(selector, {mustBeVisibleEnabled})` returns a handle or null.
  - `findAll(selector)`, `children(handle)`.
  - `get(handle, prop)`, `call(handle, method, args)`.
  - `mouseClick(handle | index, x?, y?, button, modifiers)`, `doubleClick`, `typeText(handle, text)`,
    `keyPress`.
  - `activateItem(handle, text)` for menus and actions, `clickTab(handle, text)`.
  - `clipboardText()`, `screenshot(path)`, `activeModalWidget()`, `pid()`.
- **Handles:** integer ids mapped to `QPointer<QObject>`, or to `QPersistentModelIndex` for items. A handle
  whose object has died answers like Squish does: an error on use, `"<null>"` for `str()`.
- **Selector resolution** follows Squish's documented semantics for the keys listed in Constraints:
  - `type` is matched against the `QMetaObject` class name.
  - `name` is matched against `objectName`.
  - `window` is the top-level ancestor that matches the nested selector.
  - `container` is an ancestor that matches. For `QModelIndex` items it is the view or the parent index.
  - `unnamed: 1` means `objectName` is empty.
  - `visible` is `isVisible()`.
  - `aboveWidget`/`leftWidget` compare geometry against the resolved reference widget.
  - Values may be exact strings, numbers or booleans, a regular expression, or a wildcard.
  - Nested selectors resolve recursively.
- **Properties and methods:**
  - Reads go through `QMetaProperty`. `QObject*` values come back as handles.
  - Calls go through `QMetaObject::invokeMethod` for slots and `Q_INVOKABLE` methods.
  - A small explicit allowlist covers non-invokable methods the tests use, starting with
    `QAbstractItemView::model()`, `QAbstractItemModel::rowCount/columnCount/data/index`, and
    `QApplication::activeModalWidget()`. A method outside the allowlist fails loudly.
- **Qt Quick:**
  - Below a `QQuickWidget`, resolution continues into `rootObject()` and its `QQuickItem` children.
  - `type` matches the QML type name as Squish reports it (`TextField`, `Label`, …), not the C++ class.
    The builder checks how Squish derives it for `QQuickText` and for Controls types.
  - `id` matches `QQmlContext::nameForObject`.
  - Clicks and typing map item coordinates into the scene and are delivered to the `QQuickWidget`.
  - The probe links Qt Quick and Qt Qml. The client already depends on both.
- **Input:** `QTest` mouse and key events at the widget centre, or at the given offset. Items use the
  view's `visualRect`. `typeText` understands Squish key syntax (`<Return>`, `<Ctrl+A>`, …).

### 2. Python harness (`test/gui/harness/`)

- **Modules** on `sys.path`: `squish`, `test`, `object`, `objectmaphelper`, plus a virtual `python/bdd.py`
  for `findFile`.
  - `squish.Qt` carries the enum values in use.
  - `testSettings` and `RegularExpression`/`Wildcard` serialise into selectors.
- **Proxies:** `__getattr__` reads a property. If the name is a method, it returns a callable that does a
  remote call. `hasattr` works. Python-side equality compares by handle.
- **Waiting:**
  - `waitForObject` polls until the object exists, is visible and is enabled.
  - `waitForObjectExists` only requires existence.
  - The default timeout is 20 s, from `testSettings.waitForObjectTimeout`, overridable per call.
  - A timeout raises `LookupError`.
  - `waitFor(cond, timeout)` and `snooze` are local.
- **`startApplication(cmd)`:**
  - Resolves the executable through `--aut-path`, which replaces `server.ini`.
  - Launches it with `QT_GUI_TEST_PROBE` set and connects, with a timeout. The launch also gets the
    suite's `ENVVARS` and the runner's `--envvar` values.
  - Registers an application context with `pid`, `isRunning` and `detach()`.
  - Feeds `applicationContextList()` and `currentApplicationContext()`.
- **`nativeType`** types into the focused window through the probe. The builder checks how
  `AccountSetting.py:138` uses it. If the target is a native, non-Qt dialog, it uses `xdotool` on Linux.
- **The `test` module** records results per scenario: `compare`, `verify`, `fail`, `log`, `warning`, `resultCount`.
  - With `testSettings.throwOnFailure` set, a failure raises immediately.
- **Runner CLI** (`python3 -m harness`), with flags close to `squishrunner`:
  - `--testsuite test/gui`, `--testcase tst_x` (repeatable), `--tags` with Squish tag-expression semantics
    (repeatable flags are ANDed, `~` negates), `--abortOnFail`, `--envvar K=V`, `--aut-path`.
  - Reports: `--reportgen html,<dir>` and `junitxml,<file>`.
- **Execution model:**
  - For each `tst_*` it executes `test.py` with `source`, `findFile`, `setupHooks`, `collectStepDefinitions`,
    `runFeatureFile` and the step decorators injected as builtins.
  - `.feature` files are parsed with Cucumber's `gherkin-official`, which handles Background, Scenario
    Outline/Examples, tags, doc strings and tables.
  - Squish placeholders compile to regexes (`|any|`, `|word|`, `|integer|`); `regexp=True` is used verbatim.
  - Steps match regardless of keyword, as in Squish.
  - `context.table` is a list of rows including the header, `context.multiLineText` is a list of lines, and
    `context.title` is the scenario or feature name. The builder verifies all three against the Squish
    documentation.
  - Hooks run in Squish order.
  - An undefined or ambiguous step fails the scenario with a clear message.
- **Dependencies:** pinned in `test/gui/requirements.txt`.

### 3. Harness unit tests

- `pytest` unit tests in `test/gui/harness/tests/` cover selector serialisation, tag expressions, placeholder
  compilation and the report format. They need no app or display, and they run in the `lint-gui-test` job.
- End-to-end coverage comes from the real suite, which runs on every PR again (section 4). A separate
  fixture app is not needed for now. Add one only if harness bugs turn out to be hard to tell apart from
  client bugs.

### 4. CI

- **Reverts.** The first two commits are the plain reverts, so the history shows exactly what came back:
  - `Revert "chore: disable squish tests"` (#12618), which restores `gui-tests.yml`.
  - `Revert "chore: temporary disable gui tests"` (#12620), which re-enables the branded nightly cron.
- **A shared runner script.** A new in-repo `test/gui/ci/run-gui-tests.sh` starts Xvfb (or the image's VNC
  script), D-Bus and an unlocked gnome-keyring. It then calls the harness, turning the workflow's parameters
  into harness flags. Both workflows replace `bash /dockerstartup/entrypoint.sh` with this script.
- **Changes to both workflows:**
  - Keep the `owncloudci/squish` image for its toolchain and Qt 6.8.
  - Configure with `-DWITH_GUI_TEST_PROBE=ON`.
  - Drop `LICENSEKEY` and `SERVER_INI`. `SQUISH_PARAMETERS` becomes `GUI_TEST_PARAMETERS`, with the same
    tags and the same `full-ci` rule for `--abortOnFail` in `gui-tests.yml`.
  - Report upload, stack traces and core dumps stay as they are.
- **`gui-tests.yml` hardening**, brought in line with `branded-client.yml` and the repo's zizmor check:
  - Update the action pins to the ones `branded-client.yml` uses now.
  - Add `permissions: contents: read` and `persist-credentials: false`.
  - The harness unit tests join `lint-gui-test`.
- **Verification before merge:**
  - `gui-tests.yml` runs on the PR itself, and must be green with the `~@skip ~@skipOnLinux` tag set.
  - `branded-client.yml` runs through `workflow_dispatch` on the PR branch. That branch must be in
    `owncloud/client` for the secrets to be available.
  - **Baseline:** master `fb1c873a7d`. Its scheduled run on 2026-08-01 was green with the full
    `~@skip ~@skipOnLinux` set and `--abortOnFail`, so every scenario in that set passed under Squish. The
    next day's run failed, and so did the branded job's. Run logs from then are no longer available.
  - **Prove the harness first.** On a throwaway branch, put the probe and harness on top of `fb1c873a7d`
    and run the same set. It must be fully green before master results count. Any failure there is a harness
    bug.

### 5. Open source and later extraction

The probe and the harness are developed in `owncloud/client` first. They move to their own repository once
the baseline run at `fb1c873a7d` is green; until then the interfaces change too fast for a separate release
cycle. To keep the split a mechanical `git subtree split`:

- **License: MIT** (the user's choice; it can be linked into any Qt application, including GPL-2.0-only
  ones).
  - Every new file under `test/gui/probe/` and `test/gui/harness/` starts with
    `SPDX-License-Identifier: MIT` and `Copyright (C) 2026 Thomas Müller <323649642+oc-tmueller@users.noreply.github.com>`.
  - Each of the two directories has its own `LICENSE` with the MIT text.
- **The probe is a standalone CMake project:** `test/gui/probe/CMakeLists.txt` has its own `project()` and
  `find_package(Qt6 …)` and installs a `QtGuiProbeConfig.cmake` package. The client consumes it with
  `add_subdirectory`, and after the split with `FetchContent`.
- **The harness is a standalone Python package,** with `test/gui/harness/pyproject.toml`, package `harness`
  and console script `qtguiprobe-run`. The client installs it from the path in `requirements.txt`.
- **Neutral names:**
  - C++ namespace `QtGuiProbe`, CMake target `qtguiprobe`, env var `QT_GUI_TEST_PROBE`.
  - No ownCloud names, classes or paths in either directory. Only the client's CMake wiring, the CI script
    and the workflows know about ownCloud.
  - The final project name is chosen at split time. It must not contain the Squish trademark;
    "Squish-compatible" in the description is fine.

### 6. Housekeeping

- **Changelog:** `changelog/unreleased/<PR number>.md`, type `Change`: "GUI tests run without Squish".
- **Documentation:** a `test/gui/README.md` section on running locally, covering the build flag, the harness
  command and the environment.
- Squish-specific leftovers (`ci/client_server.ini`, `SQUISH_PARAMETERS`) are removed once they're unused.

## Definition of done

- The harness passes the full `~@skip ~@skipOnLinux` set at baseline `fb1c873a7d`.
- `gui-tests.yml` is green on the PR. `branded-client.yml` passes through dispatch on the branch.
- **Scenarios that fail on master for client reasons** (the user chose this): a scenario that passes at the
  baseline but fails on master is a client regression. It gets `@skip @issue-<n>` in its `.feature` file,
  following the suite's existing tag convention.
  - `<n>` is a public GitHub issue in `owncloud/client`. It names the scenario, says it passed at
    `fb1c873a7d` and fails now, and gives the failing step. No internal ticket references.
  - The PR body lists every tag added this way. Tag edits like these are the only `.feature` changes
    allowed.
- Every edit to the test code is listed in the PR body with its reason.
- The `lint-gui-test` job passes Python lint, Gherkin lint and the harness unit tests.
- A default build (`WITH_GUI_TEST_PROBE=OFF`) contains no probe symbols. The builder checks this with `nm`
  or by grepping the binary.

## Left to the builder

- The internal structure of the C++ probe and the Python package.
- How exactly the key-syntax parser and the regex and wildcard matchers work.
- The HTML report layout. It should be readable, with one entry per scenario and step, screenshots linked.
- Whether to start X with Xvfb or the image's existing VNC startup.
