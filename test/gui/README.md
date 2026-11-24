<!--
SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
SPDX-License-Identifier: GPL-2.0-or-later
-->

# GUI tests

The GUI tests are Gherkin features (`tst_*/test.feature`) with Python steps (`shared/`). They run with
the harness in `harness/`, which starts the client and drives it through the probe in `probe/`, a
small server that the client loads when built with `-DWITH_GUI_TEST_PROBE=ON`. No Squish installation
is needed. CI runs them in `.github/workflows/gui-tests.yml` via `ci/run-gui-tests.sh`.

## Running locally

Only Linux is supported. You need an oCIS server, Python 3.10, a running X session, a D-Bus session bus
and an unlocked Secret Service keyring (gnome-keyring), since the client keeps its credentials there.

1. Build the client with the probe, from the repository root:

   ```sh
   cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Debug -DWITH_GUI_TEST_PROBE=ON
   ninja -C build
   ```

   Never ship a client built with `WITH_GUI_TEST_PROBE`. The probe stays inactive unless the harness
   sets `QT_GUI_TEST_PROBE`. To build and run the probe's own tests as well, add
   `-DWITH_GUI_TEST_PROBE_TESTS=ON` and run `ctest --test-dir build/test/gui/probe --output-on-failure`.

2. Install the Python modules (this includes the harness, installed editable from `harness/`):

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

   Use absolute paths: the harness runs each test case in its own directory. CI also sets `HOME` to the
   image's user home. Some steps create folders below `HOME`; the client's configuration and sync folders
   go to `/tmp/owncloudtest` (`XDG_CONFIG_HOME` in `envs.txt`).

4. Run a test case, from the repository root:

   ```sh
   PYTHONPATH=test/gui/harness python3.10 -m harness --testsuite test/gui --aut owncloud=build/bin --tags ~@skip --testcase tst_syncing
   ```

   Leave out `--testcase` to run every `tst_*`. Further options, all as in CI:
   `--tags ~@skipOnLinux` (repeatable, ANDed), `--abortOnFail`, `--envvar KEY=VALUE` for the client's
   environment (e.g. `--envvar QT_LOGGING_RULES=sync.httplogger=true`), and `--reportgen html,DIR` or
   `--reportgen junitxml,FILE`. `python3.10 -m harness --help` lists them all.

## Checks

```sh
make -C test/gui python-lint    # black and pylint over the suite and the harness
make -C test/gui harness-test   # the harness's own tests
make -C test/gui gherkin-lint
```
