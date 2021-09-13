import names
import squish
from helpers.SetupClientHelper import getClientDetails
import test


class AccountConnectionWizard:
    SERVER_ADDRESS_BOX = names.leUrl_OCC_PostfixLineEdit
    NEXT_BUTTON = names.owncloudWizard_qt_passive_wizardbutton1_QPushButton
    USERNAME_BOX = names.leUsername_QLineEdit
    PASSWORD_BOX = names.lePassword_QLineEdit
    SELECT_LOCAL_FOLDER = names.pbSelectLocalFolder_QPushButton
    DIRECTORY_NAME_BOX = names.fileNameEdit_QLineEdit
    CHOOSE_BUTTON = names.qFileDialog_Choose_QPushButton
    FINISH_BUTTON = {
        "name": "qt_wizard_finish",
        "type": "QPushButton",
        "visible": 1,
        "window": names.owncloudWizard_OCC_OwncloudWizard,
    }
    ERROR_OK_BUTTON = {
        "text": "OK",
        "type": "QPushButton",
        "unnamed": 1,
        "visible": 1,
        "window": names.error_QMessageBox,
    }
    CHOOSE_WHAT_TO_SYNC_BUTTON = names.bSelectiveSync_QPushButton
    CHOOSE_WHAT_TO_SYNC_CHECKBOX = names.choose_What_To_Synchronize_QTreeWidget
    CHOOSE_WHAT_TO_SYNC_OK_BUTTON = names.choose_What_To_Sync_OK_QPushButton
    MANUAL_SYNC_FOLDER = names.owncloudWizard_rManualFolder_QRadioButton

    def __init__(self):
        pass

    def sanitizeFolderPath(self, folderPath):
        return folderPath.rstrip("/")

    def addAccount(self, context):
        self.addServer(context)
        self.addUserCredentails(context)
        try:
            squish.clickButton(squish.waitForObject(self.ERROR_OK_BUTTON))
        except LookupError:
            pass
        self.changeSyncDirectory(context)
        self.connectAccount()

    def addServer(self, context):
        server = getClientDetails(context)[0]

        squish.mouseClick(squish.waitForObject(self.SERVER_ADDRESS_BOX))
        squish.type(squish.waitForObject(self.SERVER_ADDRESS_BOX), server)
        squish.clickButton(squish.waitForObject(self.NEXT_BUTTON))

    def addUserCredentails(self, context):
        _, user, password, _ = getClientDetails(context)

        squish.type(squish.waitForObject(self.USERNAME_BOX), user)
        squish.type(squish.waitForObject(self.USERNAME_BOX), "<Tab>")
        squish.type(squish.waitForObject(self.PASSWORD_BOX), password)
        squish.clickButton(squish.waitForObject(self.NEXT_BUTTON))

    def selectFoldersToSync(self, context):
        self.openSyncDialog()

        squish.mouseClick(
            squish.waitForObjectItem(self.CHOOSE_WHAT_TO_SYNC_CHECKBOX, "/"),
            11,
            11,
            squish.Qt.NoModifier,
            squish.Qt.LeftButton,
        )
        for row in context.table[1:]:
            squish.mouseClick(
                squish.waitForObjectItem(
                    self.CHOOSE_WHAT_TO_SYNC_CHECKBOX, "/." + row[0]
                ),
                11,
                11,
                squish.Qt.NoModifier,
                squish.Qt.LeftButton,
            )
        squish.clickButton(squish.waitForObject(self.CHOOSE_WHAT_TO_SYNC_OK_BUTTON))

    def changeSyncDirectory(self, context):
        localfolder = getClientDetails(context)[3]

        squish.clickButton(squish.waitForObject(self.SELECT_LOCAL_FOLDER))
        squish.mouseClick(squish.waitForObject(self.DIRECTORY_NAME_BOX))
        squish.type(squish.waitForObject(self.DIRECTORY_NAME_BOX), localfolder)
        squish.clickButton(squish.waitForObject(self.CHOOSE_BUTTON))
        test.compare(
            str(squish.waitForObjectExists(self.SELECT_LOCAL_FOLDER).text),
            self.sanitizeFolderPath(localfolder),
        )

    def connectAccount(self):
        squish.clickButton(squish.waitForObject(self.FINISH_BUTTON))

    def openSyncDialog(self):
        squish.clickButton(squish.waitForObject(self.CHOOSE_WHAT_TO_SYNC_BUTTON))

    def selectManualSyncFolder(self):
        squish.clickButton(squish.waitForObject(self.MANUAL_SYNC_FOLDER))
