<!--
SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
SPDX-License-Identifier: GPL-2.0-or-later
-->

# GUI tests

The GUI tests are Gherkin features (`tst_*/test.feature`) with Python steps (`shared/`). They run with
[CuteCumber](https://github.com/owncloud/cutecumber), which starts the client and drives it through
CuteCumber's probe, a small server that the client links when built with `-DWITH_GUI_TEST_PROBE=ON`.
No Squish installation is needed. CuteCumber's README documents the tool itself: its options, console
output, reports and the Squish API it provides. CI runs the tests in `.github/workflows/gui-tests.yml`
via `ci/run-gui-tests.sh`.

## Running locally

Only Linux is supported. You need an oCIS server, Python 3.10, a running X session, a D-Bus session bus
and an unlocked Secret Service keyring (gnome-keyring), since the client keeps its credentials there.

1. Build the client with the probe, from the repository root:

   ```sh
   cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWITH_GUI_TEST_PROBE=ON
   ninja -C build
   ```

   The configure step fetches CuteCumber's probe from GitHub, at the commit pinned in the top-level
   `CMakeLists.txt`; a build without `WITH_GUI_TEST_PROBE` fetches nothing. Never ship a client built
   with `WITH_GUI_TEST_PROBE`. The probe stays inactive unless CuteCumber sets `CUTECUMBER_PROBE`.

2. Install the Python modules (this includes CuteCumber, at the commit pinned in `requirements.txt`):

   ```sh
   python3.10 -m pip install -r test/gui/requirements.txt
   playwright install chromium
   ```

   `make -C test/gui install` does both.

3. Set the environment. These are the variables of the `Run GUI test` step in `gui-tests.yml`, with
   local values; a `test/gui/config.ini` (see `config.sample.ini`) can hold most of them instead:

   ```sh
   export GUI_TEST_REPORT_DIR="$PWD/test/gui/guiReportUpload"   # absolute
   export CLIENT_REPO=.
   export OCIS_URL=https://localhost:9200
   export BACKEND_HOST=https://localhost:9200
   export SECURE_BACKEND_HOST=https://localhost:9200
   export OCIS=true
   export STACKTRACE_FILE="$GUI_TEST_REPORT_DIR/stacktrace.log"  # absolute
   export OWNCLOUD_CORE_DUMP=1
   export RECORD_VIDEO_ON_FAILURE=false
   export COREPACK_ENABLE_STRICT=0
   mkdir -p "$GUI_TEST_REPORT_DIR/screenshots"
   ```

   Use absolute paths: CuteCumber runs each test case in its own directory. CI also sets `HOME` to the
   image's user home. Some steps create folders below `HOME`; the client's configuration and sync folders
   go to `/tmp/owncloudtest` (`XDG_CONFIG_HOME` in `envs.txt`).

4. Run a test case, from the repository root:

   ```sh
   python3.10 -m cutecumber --testsuite test/gui --aut owncloud=build/bin --tags ~@skip --testcase tst_syncing
   ```

   Leave out `--testcase` to run every `tst_*`. Further options, all as in CI:
   `--tags ~@skipOnLinux` (repeatable, ANDed), `--abortOnFail`, `--envvar KEY=VALUE` for the client's
   environment (e.g. `--envvar QT_LOGGING_RULES=sync.httplogger=true`), and `--reportgen html,DIR` or
   `--reportgen junitxml,FILE`. `python3.10 -m cutecumber --help` lists them all.

   The console shows each feature, scenario and step as it finishes, with the failing steps' lines and
   messages after each scenario that did not pass, and a summary at the end. `--console quiet` prints
   none of it, and in GitHub Actions (`GITHUB_ACTIONS=true`) `--console` defaults to `github`, which
   prints each scenario when it ends, together with what the tests printed while it ran: a passing
   scenario is one collapsed group with its result in the title, a failing one is shown open with an
   error annotation at its step, output between scenarios is a collapsed `setup output` group, and the
   job summary gets a table per feature. Lines the tests or the client print never take effect as
   workflow commands. `--console pretty` prints the plain form there too.

   The client writes its log to its standard output (`--logfile -`), so by default it shows up between
   the test results. `--aut-log-dir DIR` writes the standard output and error of each client launch to
   `DIR/<test case>/<NN>-<name>.log` instead (`NN` counts the launches in that test case), and the HTML
   report links each scenario's logs. CI passes it and uploads the logs as an artifact. A failed launch
   still quotes the last lines of the client's standard error in its error message.

## Checks

```sh
make -C test/gui python-lint    # black and pylint over the suite
make -C test/gui gherkin-lint
```
