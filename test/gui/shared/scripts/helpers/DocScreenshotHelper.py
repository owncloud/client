# -*- coding: utf-8 -*-
#
# Helper for capturing documentation screenshots.
#
# This is NOT part of the regular GUI test suite. It is used only by the
# tst_docScreenshots suite (run on demand) to regenerate the screenshots that
# ship in the owncloud/docs-client-desktop manual.
#
# It reuses the existing Squish capture path but, by default, crops to the
# relevant top-level window (grabWidget) so the documentation images do not
# include the surrounding desktop/window-manager chrome. If the window cannot
# be grabbed it falls back to a full-desktop capture so a run never silently
# produces nothing.

import os

import names
import test
import squish


# Maps a logical screen id (used in the .feature files) to the file name used
# by the documentation. Keep this in sync with the image:: macros in
# docs-client-desktop modules/ROOT/images/. Only screens reachable by the
# Squish page objects belong here; OS-native surfaces (Explorer, installer,
# Storage Sense, crash reporter, external browser) are captured manually and
# are deliberately absent.
SCREENSHOT_REGISTRY = {
    # using.adoc
    "account-settings": "using/oc-windows-account-settings.png",
    "activity-pane": "using/oc-windows-activity-pane-server.png",
    "settings-general": "using/oc-windows-settings-menu.png",
    "ignored-files-editor": "using/oc-windows-ignored-files-editor.png",
    "folder-menu": "using/oc-windows-folder-menu.png",
    "spaces-show": "using/oc-windows-ocis-resources-show.png",
    "spaces-add": "using/oc-windows-ocis-resources-add.png",
    # account connection wizard (using.adoc)
    "wizard-url": "using/oc-wizard-screen-url.png",
    "wizard-open-in-browser": "using/oc-wizard-open-in-browser.png",
    "wizard-all-set": "using/oc-wizard-all-set.png",
    "wizard-all-set-advanced": "using/oc-wizard-all-set-advanced.png",
    # troubleshooting.adoc
    "log-output-window": "troubleshooting/oc-windows-log-output-window.png",
}

# Maps a screen id to the top-level window object it should be cropped to.
# Screens not listed here are captured as a full desktop screenshot (use that
# for transient/native surfaces such as context menus). The window references
# come from test/gui/shared/scripts/names.py.
SCREENSHOT_WINDOW = {
    "account-settings": names.settings_OCC_SettingsDialog,
    "activity-pane": names.settings_OCC_SettingsDialog,
    "settings-general": names.settings_OCC_SettingsDialog,
    "spaces-show": names.settings_OCC_SettingsDialog,
    "spaces-add": names.add_Folder_Sync_Connection_OCC_FolderWizard,
    "wizard-url": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    "wizard-open-in-browser": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    "wizard-all-set": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    "wizard-all-set-advanced": names.welcome_to_ownCloud_OCC_NewAccountWizard,
}


def get_doc_screenshots_path():
    # Sub-directory of the standard report dir so the CI artifact upload picks
    # it up without extra wiring.
    return os.path.join(get_config_report_dir(), "doc-screenshots")


def get_config_report_dir():
    # Imported lazily so this module can be imported outside a running test.
    from helpers.ConfigHelper import get_config

    return get_config("guiTestReportDir")


def _resolve(screen_id):
    if screen_id not in SCREENSHOT_REGISTRY:
        raise LookupError(
            f"Unknown doc screenshot id '{screen_id}'. "
            f"Add it to DocScreenshotHelper.SCREENSHOT_REGISTRY."
        )
    return SCREENSHOT_REGISTRY[screen_id]


def _target_path(rel_name):
    target = os.path.join(get_doc_screenshots_path(), rel_name)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    return target


def capture_doc_screenshot(screen_id):
    """Capture the documented screen.

    Crops to the screen's registered top-level window when one is known,
    otherwise captures the whole desktop. Always writes to the documentation
    file name from SCREENSHOT_REGISTRY.
    """
    rel_name = _resolve(screen_id)
    target = _target_path(rel_name)
    window_ref = SCREENSHOT_WINDOW.get(screen_id)

    if window_ref is not None:
        try:
            window = squish.waitForObject(window_ref)
            squish.grabWidget(window).save(
                os.path.dirname(target), os.path.basename(target)
            )
            test.log(f"Doc screenshot (window) captured: {rel_name}")
            return target
        except (LookupError, RuntimeError) as err:
            test.log(
                f"grabWidget failed for '{screen_id}' ({err}); "
                f"falling back to desktop capture"
            )

    squish.saveDesktopScreenshot(target)
    test.log(f"Doc screenshot (desktop) captured: {rel_name}")
    return target
