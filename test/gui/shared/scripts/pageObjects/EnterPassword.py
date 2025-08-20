import names
import squish


from helpers.WebUIHelper import authorize_via_webui
from helpers.ConfigHelper import get_config


class EnterPassword:
    LOGIN_CONTAINER = {
        "name": "LoginRequiredDialog",
        "type": "OCC::LoginRequiredDialog",
        "visible": 1,
    }
    LOGIN_USER_LABEL = {
        "container": names.groupBox_OCC_QmlUtils_OCQuickWidget,
        "type": "Label",
        "visible": True,
    }
    USERNAME_BOX = {
        "name": "usernameLineEdit",
        "type": "QLineEdit",
        "visible": 1,
        "window": LOGIN_CONTAINER,
    }
    PASSWORD_BOX = {
        "container": names.groupBox_OCC_QmlUtils_OCQuickWidget,
        "id": "passwordField",
        "type": "TextField",
        "visible": True,
    }
    LOGIN_BUTTON = {
        "container": names.groupBox_OCC_QmlUtils_OCQuickWidget,
        "id": "loginButton",
        "type": "Button",
        "visible": True,
    }
    LOGOUT_BUTTON = {
        "container": names.groupBox_OCC_QmlUtils_OCQuickWidget,
        "id": "logOutButton",
        "type": "Button",
        "visible": True,
    }
    COPY_URL_TO_CLIPBOARD_BUTTON = {
        "container": names.groupBox_OCC_QmlUtils_OCQuickWidget,
        "id": "copyToClipboardButton",
        "type": "Button",
        "visible": True,
    }
    TLS_CERT_WINDOW = {
        "name": "OCC__TlsErrorDialog",
        "type": "OCC::TlsErrorDialog",
        "visible": 1,
    }
    ACCEPT_CERTIFICATE_YES = {
        "text": "Yes",
        "type": "QPushButton",
        "visible": 1,
        "window": TLS_CERT_WINDOW,
    }

    def __init__(self, occurrence=1):
        if occurrence > 1 and get_config("ocis"):
            self.TLS_CERT_WINDOW.update({"occurrence": occurrence})

    def get_username(self):
        # Parse username from the login label:
        label = str(squish.waitForObjectExists(self.LOGIN_USER_LABEL).text)
        username = label.split(" ", maxsplit=2)[1]
        return username.capitalize()

    def oidc_relogin(self, username, password):
        # wait 500ms for copy button to fully load
        squish.snooze(1 / 2)
        squish.mouseClick(squish.waitForObject(self.COPY_URL_TO_CLIPBOARD_BUTTON))
        authorize_via_webui(username, password)

    def relogin(self, username, password):
        self.oidc_relogin(username, password)

    def accept_certificate(self):
        squish.clickButton(squish.waitForObject(self.ACCEPT_CERTIFICATE_YES))
