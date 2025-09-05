import names
import squish

from pageObjects.EnterPassword import EnterPassword

from helpers.WebUIHelper import authorize_via_webui
from helpers.SetupClientHelper import (
    create_user_sync_path,
    get_temp_resource_path,
    set_current_user_sync_path,
)
from helpers.SyncHelper import listen_sync_status_for_item


class AccountConnectionWizard:
    SERVER_ADDRESS_BOX = {
        "aboveWidget": names.server_address_QLabel,
        "type": "QLineEdit",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    NEXT_BUTTON = {
        "container": names.settings_dialogStack_QStackedWidget,
        "name": "nextButton",
        "type": "QPushButton",
        "visible": 1,
    }
    SIGN_IN_BUTTON = {
        "name": "__qt__passive_wizardbutton1",
        "type": "QPushButton",
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    FINISH_BUTTON = {
        "name": "qt_wizard_finish",
        "type": "QPushButton",
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    ADVANCED_CONFIGURATION_BUTTON = {
        "name": "__qt__passive_wizardbutton6",
        "type": "QPushButton",
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    CONFIRM_INSECURE_CONNECTION_BUTTON = {
        "text": "Confirm",
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.insecure_connection_QMessageBox,
    }
    USERNAME_BOX = {
        "container": names.contentWidget_OCC_QmlUtils_OCQuickWidget,
        "id": "userNameField",
        "type": "TextField",
        "visible": True,
    }
    PASSWORD_BOX = {
        "container": names.contentWidget_OCC_QmlUtils_OCQuickWidget,
        "id": "passwordField",
        "type": "TextField",
        "visible": True,
    }
    SELECT_LOCAL_FOLDER = {
        "aboveWidget": names.welcome_to_ownCloud_Folder_location_QLabel,
        "type": "QLineEdit",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    DIRECTORY_NAME_BOX = {
        "text": "Browse...",
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    CHOOSE_BUTTON = {
        "text": "Choose",
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.qFileDialog_QFileDialog,
    }
    ERROR_LABEL = {
        "name": "errorMessageLabel",
        "type": "QLabel",
        "visible": 1,
        "window": names.setupWizardWindow_OCC_Wizard_SetupWizardWindow,
    }
    BASIC_CREDENTIAL_PAGE = {
        "container": names.contentWidget_contentWidget_QStackedWidget,
        "type": "OCC::Wizard::BasicCredentialsSetupWizardPage",
        "visible": 1,
    }
    OAUTH_CREDENTIAL_PAGE = {
        "type": "QWizardPage",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    COPY_URL_TO_CLIPBOARD_BUTTON = {
        "aboveWidget": names.leave_screen_QLabel,
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    CONF_SYNC_MANUALLY_RADIO_BUTTON = {
        "text": "Sync and download folders manually",
        "type": "QRadioButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }
    ADVANCED_CONFIGURATION_CHECKBOX = {
        "container": names.setupWizardWindow_contentWidget_QStackedWidget,
        "name": "advancedConfigGroupBox",
        "type": "QGroupBox",
        "visible": 1,
    }
    DIRECTORY_NAME_EDIT_BOX = {
        "buddy": names.qFileDialog_fileNameLabel_QLabel,
        "name": "fileNameEdit",
        "type": "QLineEdit",
        "visible": 1,
    }
    VIRTUAL_FILE_RADIO_BUTTON = {
        "container": names.advancedConfigGroupBox_syncModeGroupBox_QGroupBox,
        "name": "useVfsRadioButton",
        "type": "QRadioButton",
        "visible": 1,
    }
    SYNC_EVERYTHING_RADIO_BUTTON = {
        "text": "Automatically sync and download folders and files",
        "type": "QRadioButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.welcome_to_ownCloud_OCC_NewAccountWizard,
    }

    @staticmethod
    def add_server(server_url):
        squish.mouseClick(
            squish.waitForObject(AccountConnectionWizard.SERVER_ADDRESS_BOX)
        )
        squish.type(
            squish.waitForObject(AccountConnectionWizard.SERVER_ADDRESS_BOX),
            server_url,
        )
        AccountConnectionWizard.sign_in()

    @staticmethod
    def accept_certificate():
        squish.clickButton(squish.waitForObject(EnterPassword.ACCEPT_CERTIFICATE_YES))

    @staticmethod
    def add_user_credentials(username, password):
        AccountConnectionWizard.oidc_login(username, password)

    @staticmethod
    def oidc_login(username, password):
        AccountConnectionWizard.browser_login(username, password)

    @staticmethod
    def browser_login(username, password):
        # wait 500ms for copy button to fully load
        squish.snooze(1 / 2)
        squish.mouseClick(
            squish.waitForObject(AccountConnectionWizard.COPY_URL_TO_CLIPBOARD_BUTTON)
        )
        authorize_via_webui(username, password)

    @staticmethod
    def next_step():
        squish.clickButton(
            squish.waitForObjectExists(AccountConnectionWizard.NEXT_BUTTON)
        )

    @staticmethod
    def finish():
        squish.clickButton(
            squish.waitForObjectExists(AccountConnectionWizard.FINISH_BUTTON)
        )

    @staticmethod
    def sign_in():
        squish.clickButton(
            squish.waitForObjectExists(AccountConnectionWizard.SIGN_IN_BUTTON)
        )

    @staticmethod
    def select_sync_folder(user):
        # create sync folder for user
        sync_path = create_user_sync_path(user)

        AccountConnectionWizard.select_advanced_config()

        squish.mouseClick(
            squish.waitForObject(AccountConnectionWizard.DIRECTORY_NAME_BOX)
        )
        squish.type(
            squish.waitForObject(AccountConnectionWizard.DIRECTORY_NAME_EDIT_BOX),
            sync_path,
        )
        squish.clickButton(squish.waitForObject(AccountConnectionWizard.CHOOSE_BUTTON))
        return sync_path

    @staticmethod
    def set_temp_folder_as_sync_folder(folder_name):
        sync_path = get_temp_resource_path(folder_name)

        # clear the current path
        squish.mouseClick(
            squish.waitForObject(AccountConnectionWizard.SELECT_LOCAL_FOLDER)
        )

        squish.waitForObject(AccountConnectionWizard.SELECT_LOCAL_FOLDER).setText("")

        squish.type(
            squish.waitForObject(AccountConnectionWizard.SELECT_LOCAL_FOLDER),
            sync_path,
        )
        set_current_user_sync_path(sync_path)
        return sync_path

    @staticmethod
    def add_account(account_details):
        AccountConnectionWizard.add_account_information(account_details)
        AccountConnectionWizard.finish()

    @staticmethod
    def add_account_information(account_details):
        if account_details["server"]:
            AccountConnectionWizard.add_server(account_details["server"])
            AccountConnectionWizard.accept_certificate()
        if account_details["user"]:
            AccountConnectionWizard.add_user_credentials(
                account_details["user"],
                account_details["password"],
            )
        sync_path = ""
        if account_details["sync_folder"]:
            AccountConnectionWizard.select_advanced_config()
            sync_path = AccountConnectionWizard.set_temp_folder_as_sync_folder(
                account_details["sync_folder"]
            )
        elif account_details["user"]:
            sync_path = AccountConnectionWizard.select_sync_folder(
                account_details["user"]
            )
        if sync_path:
            # listen for sync status
            listen_sync_status_for_item(sync_path)

    @staticmethod
    def select_manual_sync_folder_option():
        squish.clickButton(
            squish.waitForObject(
                AccountConnectionWizard.CONF_SYNC_MANUALLY_RADIO_BUTTON
            )
        )

    @staticmethod
    def select_vfs_option():
        squish.clickButton(
            squish.waitForObject(AccountConnectionWizard.VIRTUAL_FILE_RADIO_BUTTON)
        )

    @staticmethod
    def select_download_everything_option():
        squish.clickButton(
            squish.waitForObject(AccountConnectionWizard.SYNC_EVERYTHING_RADIO_BUTTON)
        )

    @staticmethod
    def get_error_message():
        return str(squish.waitForObjectExists(AccountConnectionWizard.ERROR_LABEL).text)

    @staticmethod
    def is_new_connection_window_visible():
        visible = False
        try:
            squish.waitForObject(AccountConnectionWizard.SERVER_ADDRESS_BOX)
            visible = True
        except:
            pass
        return visible

    @staticmethod
    def is_credential_window_visible():
        visible = False
        try:
            squish.waitForObject(AccountConnectionWizard.OAUTH_CREDENTIAL_PAGE)
            visible = True
        except:
            pass
        return visible

    @staticmethod
    def select_advanced_config():
        squish.clickButton(
            squish.waitForObjectExists(
                AccountConnectionWizard.ADVANCED_CONFIGURATION_BUTTON
            )
        )

    @staticmethod
    def can_change_local_sync_dir():
        can_change = False
        try:
            squish.waitForObjectExists(AccountConnectionWizard.SELECT_LOCAL_FOLDER)
            squish.clickButton(
                squish.waitForObject(AccountConnectionWizard.DIRECTORY_NAME_BOX)
            )
            squish.waitForObjectExists(AccountConnectionWizard.CHOOSE_BUTTON)
            can_change = True
        except:
            pass
        return can_change

    @staticmethod
    def is_sync_everything_option_checked():
        return squish.waitForObjectExists(
            AccountConnectionWizard.SYNC_EVERYTHING_RADIO_BUTTON
        ).checked

    @staticmethod
    def is_vfs_option_checked():
        return squish.waitForObjectExists(
            AccountConnectionWizard.VIRTUAL_FILE_RADIO_BUTTON
        ).checked

    @staticmethod
    def get_local_sync_path():
        return str(
            squish.waitForObjectExists(
                AccountConnectionWizard.SELECT_LOCAL_FOLDER
            ).displayText
        )
