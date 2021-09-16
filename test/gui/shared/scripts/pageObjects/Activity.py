import names
import squish
from objectmaphelper import RegularExpression
from helpers.FilesHelper import buildConflictedRegex


class Activity:
    SUBTAB_CONTAINER = {
        "container": names.settings_stack_QStackedWidget,
        "name": "qt_tabwidget_tabbar",
        "type": "QTabBar",
        "visible": 1,
    }

    SUBTAB = {
        "container": names.settings_stack_QStackedWidget,
        "type": "QTabWidget",
        "unnamed": 1,
        "visible": 1,
    }

    def clickTab(self, tabName):
        # TODO: find some way to dynamically select the tab name
        # It might take some time for all files to sync except the expected number of unsynced files
        squish.snooze(10)

        tabFound = False

        tabCount = squish.waitForObjectExists(self.SUBTAB_CONTAINER).count
        for index in range(tabCount):
            tabText = squish.waitForObjectExists(
                {
                    "container": names.stack_qt_tabwidget_tabbar_QTabBar,
                    "index": index,
                    "type": "TabItem",
                }
            ).text

            if tabName in tabText:
                tabFound = True
                squish.clickTab(squish.waitForObject(self.SUBTAB), tabText)
                break

        if not tabFound:
            raise Exception("Tab not found: " + tabName)

    def checkFileExist(self, filename):
        squish.waitForObject(names.settings_OCC_SettingsDialog)
        squish.waitForObjectExists(
            {
                "column": 1,
                "container": names.oCC_IssuesWidget_tableView_QTableView,
                "text": RegularExpression(buildConflictedRegex(filename)),
                "type": "QModelIndex",
            }
        )
