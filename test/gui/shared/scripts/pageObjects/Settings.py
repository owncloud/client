import names
import squish

from pageObjects.Toolbar import Toolbar


class Settings:
    GENERAL_CHECKBOX_OPTION_ITEM = {
        "container": names.scrollArea_generalGroupBox_QGroupBox,
        "type": "QCheckBox",
        "visible": 1,
    }
    ADVANCED_CHECKBOX_OPTION_ITEM = {
        "container": names.scrollArea_advancedGroupBox_QGroupBox,
        "type": "QCheckBox",
        "visible": 1,
    }
    NETWORK_OPTION_ITEM = {
        "container": names.scrollArea_groupBox_QGroupBox,
        "type": "QGroupBox",
        "visible": 1,
    }
    ABOUT_BUTTON = {
        "container": names.mainWindow_QMenu,
        "name": "aboutAction",
        "type": "QAction",
        "visible": True,
    }
    ABOUT_DIALOG = {
        "container": names.mainWindow_qt_tabwidget_stackedwidget_QStackedWidget,
        "name": "tab",
        "type": "QWidget",
        "visible": 1,
    }
    ABOUT_DIALOG_OK_BUTTON = {
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": ABOUT_DIALOG,
    }
    CLOSE_BUTTON = {
        "name": "closeButton",
        "type": "QPushButton",
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }

    GENERAL_OPTIONS_MAP = {
        "Start on Login": "autostartCheckBox",
        "Use Monochrome Icons in the system tray": "monoIconsCheckBox",
        "Language": "languageDropdown",
        "Show desktop Notifications": "desktopNotificationsCheckBox",
    }
    ADVANCED_OPTION_MAP = {
        "Sync hidden files": "syncHiddenFilesCheckBox",
        "Show crash reporter": "",
        "Edit ignored files": "ignoredFilesButton",
        "Log settings": "logSettingsButton",
        "Ask for confirmation before synchronizing folders larger than 500 MB": "newFolderLimitCheckBox",
        "Ask for confirmation before synchronizing external storages": "newExternalStorage",
    }
    NETWORK_OPTION_MAP = {
        "Proxy Settings": "proxyGroupBox",
        "Download Bandwidth": "downloadBox",
        "Upload Bandwidth": "uploadBox",
    }

    @staticmethod
    def get_general_checkbox_option_selector(name):
        selector = Settings.GENERAL_CHECKBOX_OPTION_ITEM.copy()
        selector.update({"name": name})
        if name == "languageDropdown":
            selector.update({"type": "QComboBox"})
        return selector

    @staticmethod
    def get_advanced_checkbox_option_selector(name):
        selector = Settings.ADVANCED_CHECKBOX_OPTION_ITEM.copy()
        selector.update({"name": name})
        if name in ("ignoredFilesButton", "logSettingsButton"):
            selector.update({"type": "QPushButton"})
        return selector

    @staticmethod
    def get_network_option_selector(name):
        selector = Settings.NETWORK_OPTION_ITEM.copy()
        selector.update({"name": name})
        return selector

    @staticmethod
    def check_general_option(option):
        selector = Settings.GENERAL_OPTIONS_MAP[option]
        squish.waitForObjectExists(
            Settings.get_general_checkbox_option_selector(selector)
        )

    @staticmethod
    def check_advanced_option(option):
        selector = Settings.ADVANCED_OPTION_MAP[option]
        squish.waitForObjectExists(
            Settings.get_advanced_checkbox_option_selector(selector)
        )

    @staticmethod
    def check_network_option(option):
        selector = Settings.NETWORK_OPTION_MAP[option]
        squish.waitForObjectExists(Settings.get_network_option_selector(selector))

    @staticmethod
    def open_about_button():
        squish.mouseClick(squish.waitForObject(Toolbar.TOOLBAR_MORE_BUTTON))
        squish.mouseClick(squish.waitForObject(Settings.ABOUT_BUTTON))

    @staticmethod
    def wait_for_about_dialog_to_be_visible():
        squish.waitForObjectExists(Settings.ABOUT_DIALOG)

    @staticmethod
    def close_about_dialog():
        squish.clickButton(squish.waitForObjectExists(Settings.ABOUT_DIALOG_OK_BUTTON))

    @staticmethod
    def close_settings_tab():
        squish.clickButton(squish.waitForObjectExists(Settings.CLOSE_BUTTON))
