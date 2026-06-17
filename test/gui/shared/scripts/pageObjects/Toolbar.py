import names
import squish
import object  # pylint: disable=redefined-builtin
from objectmaphelper import RegularExpression

from helpers.SetupClientHelper import wait_until_app_killed
from helpers.ConfigHelper import get_config


class Toolbar:
    TOOLBAR_ROW = {
        "name": "mainWindowToolbar",
        "type": "QToolBar",
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }
    ACCOUNT_BUTTON = {
        "type": "QToolButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }
    ADD_ACCOUNT_BUTTON = {
        "container": names.mainWindow_QMenu,
        "name": "addAcountAction",
        "type": "QAction",
        "visible": True,
    }
    ACTIVITY_BUTTON = {
        "text": "Activity",
        "type": "QToolButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }
    ERRORS_BUTTON = {
        "text": RegularExpression(r"^Errors: [0-9]+$"),
        "type": "QToolButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }
    SETTINGS_BUTTON = {
        "container": names.mainWindow_QMenu,
        "name": "settingsAction",
        "type": "QAction",
        "visible": True,
    }
    QUIT_BUTTON = {
        "container": names.mainWindow_QMenu,
        "name": "quitAction",
        "type": "QAction",
        "visible": True,
    }
    QUIT_CONFIRMATION_DIALOG = {
        "type": "QMessageBox",
        "unnamed": 1,
        "visible": 1,
    }
    CONFIRM_QUIT_BUTTON = {
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": QUIT_CONFIRMATION_DIALOG,
    }
    TOOLBAR_MORE_BUTTON = {
        "text": "More",
        "type": "QToolButton",
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }

    TOOLBAR_ITEMS = ["Add Account", "Activity", "Settings", "Quit"]

    @staticmethod
    def get_item_selector(item_name):
        return {
            "window": names.mainWindow_OCC_MainWindow,
            "text": item_name,
            "type": "QToolButton",
            "visible": True,
        }

    @staticmethod
    def has_item(item_name, timeout=get_config("minSyncTimeout") * 1000):
        try:
            squish.waitForObject(Toolbar.get_item_selector(item_name), timeout)
            return True
        except:
            return False

    @staticmethod
    def open_activity():
        squish.mouseClick(squish.waitForObject(Toolbar.ACTIVITY_BUTTON))

    @staticmethod
    def open_errors():
        squish.mouseClick(squish.waitForObject(Toolbar.ERRORS_BUTTON))

    @staticmethod
    def open_new_account_setup():
        squish.mouseClick(squish.waitForObject(Toolbar.TOOLBAR_MORE_BUTTON))
        squish.mouseClick(squish.waitForObject(Toolbar.ADD_ACCOUNT_BUTTON))

    @staticmethod
    def open_account(displayname):
        _, selector = Toolbar.get_account(displayname)
        squish.mouseClick(squish.waitForObject(selector))

    @staticmethod
    def get_displayed_account_text(displayname, host):
        return str(
            squish.waitForObjectExists(
                Toolbar.get_item_selector(displayname + "\n" + host)
            ).text
        )

    @staticmethod
    def open_settings_tab():
        squish.mouseClick(squish.waitForObject(Toolbar.TOOLBAR_MORE_BUTTON))
        squish.mouseClick(squish.waitForObject(Toolbar.SETTINGS_BUTTON))

    @staticmethod
    def quit_owncloud():
        squish.mouseClick(squish.waitForObject(Toolbar.TOOLBAR_MORE_BUTTON))
        squish.mouseClick(squish.waitForObject(Toolbar.QUIT_BUTTON))
        squish.clickButton(squish.waitForObject(Toolbar.CONFIRM_QUIT_BUTTON))
        for ctx in squish.applicationContextList():
            pid = ctx.pid
            ctx.detach()
            wait_until_app_killed(pid)

    @staticmethod
    def get_accounts():
        accounts = {}
        selectors = {}
        children_obj = object.children(squish.waitForObjectExists(Toolbar.TOOLBAR_ROW))
        account_idx = 1
        for obj in children_obj:

            if hasattr(obj, "defaultAction") and hasattr(
                obj.defaultAction, "objectName"
            ):
                object_name = str(getattr(obj.defaultAction, "objectName", ""))
                if not object_name.startswith("accountAction_"):
                    continue
                hostname = str(obj.text).strip()
                tooltip = str(obj.toolTip).strip()
                lines = [line.strip() for line in tooltip.splitlines() if line.strip()]
                username = lines[0] if lines else ""
                account_info = {
                    "displayname": username,
                    "username": username,
                    "hostname": hostname,
                    "initials": "",
                    "current": getattr(obj, "checked", False),
                }
                account_locator = Toolbar.ACCOUNT_BUTTON.copy()
                if account_idx > 1:
                    account_locator.update({"occurrence": account_idx})
                account_locator.update({"text": hostname})

                accounts[username] = account_info
                selectors[username] = obj
                account_idx += 1
        return accounts, selectors

    @staticmethod
    def get_account(display_name):
        accounts, selectors = Toolbar.get_accounts()
        return accounts.get(display_name), selectors.get(display_name)

    @staticmethod
    def get_active_account():
        accounts, selectors = Toolbar.get_accounts()
        for account, info in accounts.items():
            if info["current"]:
                return info, selectors[account]
        return None, None

    @staticmethod
    def account_has_focus(display_name):
        account, selector = Toolbar.get_account(display_name)
        return account["current"] and squish.waitForObject(selector).checked

    @staticmethod
    def account_exists(display_name):
        account, selector = Toolbar.get_account(display_name)
        if (
            account is None
            or selector is None
            and account["displayname"] != display_name
        ):
            raise LookupError(f'Account "{display_name}" does not exist')
        squish.waitForObject(selector)
