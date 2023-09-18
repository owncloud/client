import squish
import names


class Toolbar:
    QUIT_OWNCLOUD_YES_QPUSHBUTTON = {
        "text": "Yes",
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.quit_ownCloud_QMessageBox,
    }

    @staticmethod
    def getItemSelector(item_name):
        return {
            "name": "settingsdialog_toolbutton_%s" % item_name,
            "type": "QToolButton",
            "visible": 1,
        }

    @staticmethod
    def openActivity():
        squish.clickButton(squish.waitForObject(Toolbar.getItemSelector("Activity")))

    @staticmethod
    def openNewAccountSetup():
        squish.clickButton(squish.waitForObject(Toolbar.getItemSelector("Add account")))

    @staticmethod
    def openAccount(displayname, host):
        squish.clickButton(
            squish.waitForObject(Toolbar.getItemSelector(displayname + "@" + host))
        )

    @staticmethod
    def getDisplayedAccountText(displayname, host):
        return str(
            squish.waitForObjectExists(
                Toolbar.getItemSelector(displayname + "@" + host)
            ).text
        )

    @staticmethod
    def open_settings_tab():
        squish.clickButton(squish.waitForObject(Toolbar.getItemSelector("Settings")))

    @staticmethod
    def quitOwncloud():
        squish.clickButton(
            squish.waitForObject(Toolbar.getItemSelector("Quit ownCloud"))
        )
        squish.clickButton(squish.waitForObject(Toolbar.QUIT_OWNCLOUD_YES_QPUSHBUTTON))
