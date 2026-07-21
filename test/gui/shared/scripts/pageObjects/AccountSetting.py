import names
import squish


class AccountSetting:
    MANAGE_ACCOUNT_BUTTON = {
        "container": names.mainWindow_OCC_MainWindow,
        "name": "manageAccountButton",
        "type": "QPushButton",
        "visible": 1,
    }
    ACCOUNT_MENU = {
        "container": names.mainWindow_manageAccountMenu_QMenu,
        "type": "QAction",
        "visible": True,
    }
    CONFIRM_REMOVE_CONNECTION_BUTTON = {
        "name": "removeAccountButton",
        "type": "QPushButton",
        "visible": 1,
        "window": names.confirmRemoveAccountDialog_QMessageBox,
    }
    ACCOUNT_CONNECTION_LABEL = {
        "container": names.mainWindow_OCC_MainWindow,
        "name": "connectionStatusLabel",
        "type": "QLabel",
        "visible": 1,
    }
    LOG_BROWSER_WINDOW = {
        "name": "OCC__LogBrowser",
        "type": "OCC::LogBrowser",
        "visible": 1,
    }
    ACCOUNT_LOADING = {
        "window": names.mainWindow_OCC_MainWindow,
        "name": "loadingPage",
        "type": "QWidget",
        "visible": 0,
    }
    DIALOG_STACK = {
        "name": "dialogStack",
        "type": "QStackedWidget",
        "visible": 1,
        "window": names.mainWindow_OCC_MainWindow,
    }
    CONFIRMATION_YES_BUTTON = {"type": "QPushButton", "visible": 1}

    @staticmethod
    def account_action(action, name):
        squish.clickButton(squish.waitForObject(AccountSetting.MANAGE_ACCOUNT_BUTTON))
        action_selector = AccountSetting.ACCOUNT_MENU.copy()
        action_selector.update({"text": action, "name": name})
        squish.activateItem(squish.waitForObject(action_selector))

    @staticmethod
    def remove_account_connection():
        AccountSetting.account_action("Remove", "removeAction")
        squish.clickButton(
            squish.waitForObject(AccountSetting.CONFIRM_REMOVE_CONNECTION_BUTTON)
        )

    @staticmethod
    def logout():
        AccountSetting.account_action("Log out", "logInOutAction")

    @staticmethod
    def login():
        AccountSetting.account_action("Log in", "logInOutAction")

    @staticmethod
    def get_account_connection_label():
        return str(
            squish.waitForObjectExists(AccountSetting.ACCOUNT_CONNECTION_LABEL).text
        )

    @staticmethod
    def is_connecting():
        return "Connecting to" in AccountSetting.get_account_connection_label()

    @staticmethod
    def is_user_signed_out():
        return "Signed out" in AccountSetting.get_account_connection_label()

    @staticmethod
    def is_user_signed_in():
        return "Connected" in AccountSetting.get_account_connection_label()

    @staticmethod
    def wait_until_connection_is_configured(timeout=5000):
        result = squish.waitFor(
            AccountSetting.is_connecting,
            timeout,
        )

        if not result:
            raise TimeoutError(
                "Timeout waiting for connection to be configured for "
                + str(timeout)
                + " milliseconds"
            )

    @staticmethod
    def wait_until_account_is_connected(timeout=5000):
        result = squish.waitFor(
            AccountSetting.is_user_signed_in,
            timeout,
        )

        if not result:
            raise TimeoutError(
                "Timeout waiting for the account to be connected for "
                + str(timeout)
                + " milliseconds"
            )
        return result

    @staticmethod
    def wait_until_sync_folder_is_configured(timeout=5000):
        result = squish.waitFor(
            lambda: not squish.waitForObjectExists(
                AccountSetting.ACCOUNT_LOADING
            ).visible,
            timeout,
        )

        if not result:
            raise TimeoutError(
                "Timeout waiting for sync folder to be connected for "
                + str(timeout)
                + " milliseconds"
            )
        return result

    @staticmethod
    def press_key(key):
        key = key.replace('"', "")
        key = f"<{key}>"
        squish.nativeType(key)

    @staticmethod
    def is_log_dialog_visible():
        visible = False
        try:
            visible = squish.waitForObjectExists(
                AccountSetting.LOG_BROWSER_WINDOW
            ).visible
        except:
            pass
        return visible
