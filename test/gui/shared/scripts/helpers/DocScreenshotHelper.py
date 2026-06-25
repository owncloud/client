# -*- coding: utf-8 -*-
#
# Helper for capturing documentation screenshots.
#
# This is NOT part of the regular GUI test suite. It is used only by the
# tst_docScreenshots suite (run on demand) to regenerate the screenshots that
# ship in the owncloud/docs-client-desktop manual.
#
# It reuses the existing Squish capture path (saveDesktopScreenshot, the same
# call ReportHelper.take_screenshot uses on failure) but writes to a dedicated
# directory and resolves names through a single registry so the documentation
# file naming convention (oc-*) lives in exactly one place.

import os

import test
import squish

from helpers.ConfigHelper import get_config


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
    "log-files-to-keep": "troubleshooting/oc-windows-log-files-to-keep.png",
}


def get_doc_screenshots_path():
    # Sub-directory of the standard report dir so the CI artifact upload picks
    # it up without extra wiring.
    return os.path.join(get_config("guiTestReportDir"), "doc-screenshots")


def _resolve(screen_id):
    if screen_id not in SCREENSHOT_REGISTRY:
        raise LookupError(
            f"Unknown doc screenshot id '{screen_id}'. "
            f"Add it to DocScreenshotHelper.SCREENSHOT_REGISTRY."
        )
    return SCREENSHOT_REGISTRY[screen_id]


def capture_doc_screenshot(screen_id):
    """Capture the current desktop and save it under the documentation name."""
    rel_name = _resolve(screen_id)
    directory = get_doc_screenshots_path()
    target = os.path.join(directory, rel_name)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    squish.saveDesktopScreenshot(target)
    test.log(f"Doc screenshot captured: {rel_name}")
    return target


def capture_widget_screenshot(screen_id, object_ref):
    """Capture a single widget/window (cropped) rather than the whole desktop.

    Use this for surfaces where the surrounding desktop is noise (for example
    the system tray menu or a small dialog). Falls back to a desktop capture if
    the object cannot be grabbed.
    """
    rel_name = _resolve(screen_id)
    directory = get_doc_screenshots_path()
    target = os.path.join(directory, rel_name)
    os.makedirs(os.path.dirname(target), exist_ok=True)
    try:
        obj = squish.waitForObject(object_ref)
        squish.grabWidget(obj).save(directory, os.path.basename(rel_name))
        test.log(f"Doc screenshot (widget) captured: {rel_name}")
    except (LookupError, RuntimeError) as err:
        test.log(f"grabWidget failed for '{screen_id}' ({err}); using desktop capture")
        squish.saveDesktopScreenshot(target)
    return target
