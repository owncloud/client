# Documentation Screenshots (on-demand)

This Squish suite regenerates the screenshots that ship in the desktop manual
([owncloud/docs-client-desktop](https://github.com/owncloud/docs-client-desktop)).

It is **not** part of the regular GUI test run. It lives on the long-lived
`tooling/doc-screenshots-*` branch, which is **never merged** to `master`.

## How it works

Each scenario in `test.feature` navigates to a documented screen using the
existing page objects (the same ones the functional tests use) and then calls:

```gherkin
Then the screenshot "<screen-id>" is captured
```

The `<screen-id>` is resolved to the documentation file name (the `oc-*`
convention) by `SCREENSHOT_REGISTRY` in
`shared/scripts/helpers/DocScreenshotHelper.py`. That registry is the single
source of truth for screen-id → file-name mapping — update it there, not in the
feature files.

Captured PNGs are written under
`<GUI_TEST_REPORT_DIR>/doc-screenshots/<page>/<oc-name>.png`.

## Running it

### CI (recommended)

Run the **Documentation Screenshots** workflow
(`.github/workflows/doc-screenshots.yml`) via *workflow_dispatch*. It builds the
client, runs only this suite against an oCIS service, and uploads the PNGs as a
`doc-screenshots-linux-<run>` artifact. Download it and copy the relevant images
into the docs repo.

> Rebase this branch onto the target release branch (e.g. `7.1`) **before**
> running, so the screenshots match the documented client version.

### Locally

Same prerequisites as the regular GUI tests (Squish, a `config.ini`, a reachable
backend). Then run only this test case, e.g.:

```
squishrunner --testsuite test/gui --testcase tst_docScreenshots ...
```

## Scope and limits

- **In scope (automated):** in-app Qt windows reachable by the page objects —
  the account connection wizard (server URL, advanced configuration), account
  settings, the spaces/sync-connections list, the Activity window, the General
  settings pane, the Ignored Files editor, the Log Output window, and the
  folder sync options (three-dots) menu.
- **Not yet automated:** the OAuth/OIDC wizard pages (`wizard-open-in-browser`,
  `wizard-all-set`) require driving the external browser login against the test
  IdP; add scenarios once that flow is wired through the WebUI helper.
- **Out of scope (manual, per platform):** OS-native surfaces that no Qt UI
  automation can drive uniformly — the OS file manager (Explorer/Finder),
  Windows virtual files / Storage Sense, the MSI installer wizard, the external
  browser / web UI, and the crash reporter. Capture these by hand on each OS.
- `faq/sync-quota-spaces.png` is intentionally **absent** from the registry: the
  per-space quota readout was removed in client 7.0 and cannot be reproduced.

## Windows / macOS

Squish supports both, and this suite and its page objects run unchanged there.
The matrix jobs in the workflow are stubbed (`if: false`) until runners with
Squish installed and a reachable backend are available.
