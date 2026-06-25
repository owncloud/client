# -*- coding: utf-8 -*-
#
# Navigation helpers used only by the on-demand documentation screenshot suite
# (tst_docScreenshots). These open screens that the regular functional tests
# never needed to navigate to on their own.
#
# Selectors are derived from the existing object map (names.py) and the client
# UI (generalsettings.ui buttons, the OCC::IgnoreListEditor / OCC::LogBrowser
# dialogs). They intentionally avoid the keyboard-shortcut path for the log
# window (that shortcut does not exist in the client).

import names
import squish

from pageObjects.Toolbar import Toolbar


class DocScreens:
    # Buttons on the General settings pane (generalsettings.ui).
    IGNORED_FILES_BUTTON = {
        "container": names.stack_scrollArea_QScrollArea,
        "name": "ignoredFilesButton",
        "type": "QPushButton",
        "visible": 1,
    }
    LOG_SETTINGS_BUTTON = {
        "container": names.stack_scrollArea_QScrollArea,
        "name": "logSettingsButton",
        "type": "QPushButton",
        "visible": 1,
    }

    # Modeless dialogs opened from the General settings pane.
    IGNORE_LIST_EDITOR = {
        "type": "OCC::IgnoreListEditor",
        "visible": 1,
    }
    LOG_BROWSER = {
        "type": "OCC::LogBrowser",
        "visible": 1,
    }

    @staticmethod
    def open_settings_tab():
        # The Settings tab is opened via the QML toolbar button (settingsButton),
        # the same path the functional tests use.
        Toolbar.open_settings_tab()

    @staticmethod
    def open_ignored_files_editor():
        DocScreens.open_settings_tab()
        squish.clickButton(squish.waitForObject(DocScreens.IGNORED_FILES_BUTTON))
        squish.waitForObject(DocScreens.IGNORE_LIST_EDITOR)

    @staticmethod
    def open_log_settings():
        DocScreens.open_settings_tab()
        squish.clickButton(squish.waitForObject(DocScreens.LOG_SETTINGS_BUTTON))
        squish.waitForObject(DocScreens.LOG_BROWSER)
