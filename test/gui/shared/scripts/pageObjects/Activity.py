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
        tabFound = False

        # Selecting tab by name fails for "Not Synced" when there are no unsynced files
        # Because files count will be appended like "Not Synced (2)"
        # So to overcome this the following approach has been implemented
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

    def checkConflictFileExist(self, filename):
        squish.waitForObject(names.settings_OCC_SettingsDialog)
        squish.waitForObjectExists(
            {
                "column": 1,
                "container": names.oCC_IssuesWidget_tableView_QTableView,
                "text": RegularExpression(buildConflictedRegex(filename)),
                "type": "QModelIndex",
            }
        )

    def checkNotSyncedResourceExist(self, context, filename, status):
        squish.waitForObject(names.settings_OCC_SettingsDialog)

        result = squish.waitFor(
            lambda: self.isNotSyncedResourceExist(context, filename, status),
            context.userData['maxSyncTimeout'] * 1000,
        )

        return result

    def waitForResourceNotExist(self, context, filename, status="Excluded"):
        squish.waitForObject(names.settings_OCC_SettingsDialog)

        result = squish.waitFor(
            lambda: not self.isNotSyncedResourceExist(context, filename, status),
            context.userData['maxSyncTimeout'] * 1000,
        )

        return result
    
    def isNotSyncedResourceExist(self, context, filename, status):
        try:
            # The blacklisted file does not have text like (conflicted copy) appended to it in the not synced table.
            fileRow = squish.waitForObject(
                {
                    "column": 1,
                    "container": names.oCC_IssuesWidget_tableView_QTableView,
                    "text": filename,
                    "type": "QModelIndex",
                },
                context.userData['lowestSyncTimeout'] * 1000,
            )["row"]
            squish.waitForObjectExists(
                {
                    "column": 6,
                    "row": fileRow,
                    "container": names.oCC_IssuesWidget_tableView_QTableView,
                    "text": status,
                    "type": "QModelIndex",
                },
                context.userData['lowestSyncTimeout'] * 1000,
            )
            
            return True
        except:
            return False

    def checkTableContent(self, context, action, resource, account):
        squish.waitForObject(names.settings_OCC_SettingsDialog)

        result = squish.waitFor(
            lambda: self.isTableContentMatch(context, action, resource, account),
            context.userData['maxSyncTimeout'] * 1000,
        )

        return result

    def isTableContentMatch(self, context, action, resource, account):
        try:
            fileRow = squish.waitForObject(
                {
                    "column": 1,
                    "container": names.oCC_ProtocolWidget_tableView_QTableView,
                    "text": resource,
                    "type": "QModelIndex",
                },
                context.userData['lowestSyncTimeout'] * 1000,
            )["row"]
            squish.waitForObjectExists(
                {
                    "column": 0,
                    "row": fileRow,
                    "container": names.oCC_ProtocolWidget_tableView_QTableView,
                    "text": action,
                    "type": "QModelIndex",
                },
                context.userData['lowestSyncTimeout'] * 1000,
            )
            squish.waitForObjectExists(
                {
                    "column": 4,
                    "row": fileRow,
                    "container": names.oCC_ProtocolWidget_tableView_QTableView,
                    "text": account,
                    "type": "QModelIndex",
                },
                context.userData['lowestSyncTimeout'] * 1000,
            )

            return True
        except:
            return False
