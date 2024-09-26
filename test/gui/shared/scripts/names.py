# encoding: UTF-8

from objectmaphelper import *

settings_OCC_SettingsDialog = {
    "name": "Settings",
    "type": "OCC::SettingsDialog",
    "visible": 1,
}
owncloudWizard_OCC_OwncloudWizard = {
    "name": "owncloudWizard",
    "type": "OCC::OwncloudWizard",
    "visible": 1,
}
qFileDialog_QFileDialog = {"name": "QFileDialog", "type": "QFileDialog", "visible": 1}
settings_stack_QStackedWidget = {
    "name": "stackedWidget",
    "type": "QStackedWidget",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
qFileDialog_fileNameLabel_QLabel = {
    "name": "fileNameLabel",
    "type": "QLabel",
    "visible": 1,
    "window": qFileDialog_QFileDialog,
}
sharingDialog_OCC_ShareDialog = {
    "name": "SharingDialog",
    "type": "OCC::ShareDialog",
    "visible": 1,
}
sharingDialog_qt_tabwidget_stackedwidget_QStackedWidget = {
    "name": "qt_tabwidget_stackedwidget",
    "type": "QStackedWidget",
    "visible": 1,
    "window": sharingDialog_OCC_ShareDialog,
}
qt_tabwidget_stackedwidget_SharingDialogUG_OCC_ShareUserGroupWidget = {
    "container": sharingDialog_qt_tabwidget_stackedwidget_QStackedWidget,
    "name": "SharingDialogUG",
    "type": "OCC::ShareUserGroupWidget",
    "visible": 1,
}
sharingDialogUG_scrollArea_QScrollArea = {
    "container": qt_tabwidget_stackedwidget_SharingDialogUG_OCC_ShareUserGroupWidget,
    "name": "scrollArea",
    "type": "QScrollArea",
    "visible": 1,
}
settings_settingsdialog_toolbutton_Quit_ownCloud_QToolButton = {
    "name": "settingsdialog_toolbutton_Quit ownCloud",
    "type": "QToolButton",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
settings_settingsdialog_toolbutton_Settings_QToolButton = {
    "name": "settingsdialog_toolbutton_Settings",
    "type": "QToolButton",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
stack_qt_tabwidget_stackedwidget_QStackedWidget = {
    "container": settings_stack_QStackedWidget,
    "name": "qt_tabwidget_stackedwidget",
    "type": "QStackedWidget",
    "visible": 1,
}
qt_tabwidget_stackedwidget_OCC_IssuesWidget_OCC_IssuesWidget = {
    "container": stack_qt_tabwidget_stackedwidget_QStackedWidget,
    "name": "OCC__IssuesWidget",
    "type": "OCC::IssuesWidget",
    "visible": 1,
}
sharingDialog_qt_tabwidget_tabbar_QTabBar = {
    "name": "qt_tabwidget_tabbar",
    "type": "QTabBar",
    "visible": 1,
    "window": sharingDialog_OCC_ShareDialog,
}
qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget = {
    "container": sharingDialog_qt_tabwidget_stackedwidget_QStackedWidget,
    "name": "OCC__ShareLinkWidget",
    "type": "OCC::ShareLinkWidget",
    "visible": 1,
}
oCC_ShareLinkWidget_checkBox_password_QCheckBox = {
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "name": "checkBox_password",
    "type": "QCheckBox",
    "visible": 1,
}
oCC_ShareLinkWidget_widget_editing_QWidget = {
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "name": "widget_editing",
    "type": "QWidget",
    "visible": 1,
}
oCC_ShareLinkWidget_checkBox_password_QProgressIndicator = {
    "aboveWidget": oCC_ShareLinkWidget_widget_editing_QWidget,
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "leftWidget": oCC_ShareLinkWidget_checkBox_password_QCheckBox,
    "type": "QProgressIndicator",
    "unnamed": 1,
    "visible": 1,
}
oCC_ShareLinkWidget_linkShares_QTableWidget = {
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "name": "linkShares",
    "type": "QTableWidget",
    "visible": 1,
}
oCC_ShareLinkWidget_lineEdit_password_QLineEdit = {
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "name": "lineEdit_password",
    "type": "QLineEdit",
    "visible": 1,
}
oCC_ShareLinkWidget_checkBox_expire_QCheckBox = {
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "name": "checkBox_expire",
    "type": "QCheckBox",
    "visible": 1,
}
oCC_ShareLinkWidget_checkBox_expire_QProgressIndicator = {
    "aboveWidget": oCC_ShareLinkWidget_lineEdit_password_QLineEdit,
    "container": qt_tabwidget_stackedwidget_OCC_ShareLinkWidget_OCC_ShareLinkWidget,
    "leftWidget": oCC_ShareLinkWidget_checkBox_expire_QCheckBox,
    "type": "QProgressIndicator",
    "unnamed": 1,
    "visible": 1,
}
settings_settingsdialog_toolbutton_Add_account_QToolButton = {
    "name": "settingsdialog_toolbutton_Add account",
    "type": "QToolButton",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
settings_settingsdialog_toolbutton_Activity_QToolButton = {
    "name": "settingsdialog_toolbutton_Activity",
    "type": "QToolButton",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
disable_virtual_file_support_QMessageBox = {
    "type": "QMessageBox",
    "unnamed": 1,
    "visible": 1,
    "windowTitle": "Disable virtual file support?",
}
owncloudWizard_urlLabel_QLabel = {
    "name": "urlLabel",
    "type": "QLabel",
    "visible": 1,
    "window": owncloudWizard_OCC_OwncloudWizard,
}
setupWizardWindow_OCC_Wizard_SetupWizardWindow = {
    "name": "SetupWizardWidget",
    "type": "OCC::Wizard::SetupWizardWidget",
    "visible": 1,
}
setupWizardWindow_contentWidget_QStackedWidget = {
    "name": "contentWidget",
    "type": "QStackedWidget",
    "visible": 1,
    "window": setupWizardWindow_OCC_Wizard_SetupWizardWindow,
}
insecure_connection_QMessageBox = {
    "type": "QMessageBox",
    "unnamed": 1,
    "visible": 1,
    "windowTitle": "Insecure connection",
}
contentWidget_advancedConfigGroupBox_QGroupBox = {
    "container": setupWizardWindow_contentWidget_QStackedWidget,
    "name": "advancedConfigGroupBox",
    "type": "QGroupBox",
    "visible": 1,
}
advancedConfigGroupBox_localDirectoryGroupBox_QGroupBox = {
    "container": contentWidget_advancedConfigGroupBox_QGroupBox,
    "name": "localDirectoryGroupBox",
    "type": "QGroupBox",
    "visible": 1,
}
advancedConfigGroupBox_syncModeGroupBox_QGroupBox = {
    "container": contentWidget_advancedConfigGroupBox_QGroupBox,
    "name": "syncModeGroupBox",
    "type": "QGroupBox",
    "visible": 1,
}
add_Folder_Sync_Connection_OCC_FolderWizard = {
    "type": "OCC::FolderWizard",
    "unnamed": 1,
    "visible": 1,
    "windowTitle": "Add Folder Sync Connection",
}
add_Folder_Sync_Connection_groupBox_QGroupBox = {
    "name": "groupBox",
    "type": "QGroupBox",
    "visible": 1,
    "window": add_Folder_Sync_Connection_OCC_FolderWizard,
}
add_Folder_Sync_Connection_Deselect_remote_folders_you_do_not_wish_to_synchronize_QLabel = {
    "text": "Deselect remote folders you do not wish to synchronize.",
    "type": "QLabel",
    "unnamed": 1,
    "visible": 1,
    "window": add_Folder_Sync_Connection_OCC_FolderWizard,
}
folder_Sync_Connection_Deselect_remote_folders_QTreeWidget = {
    "aboveWidget": add_Folder_Sync_Connection_Deselect_remote_folders_you_do_not_wish_to_synchronize_QLabel,
    "type": "QTreeWidget",
    "unnamed": 1,
    "visible": 1,
    "window": add_Folder_Sync_Connection_OCC_FolderWizard,
}
deselect_remote_folders_you_do_not_wish_to_synchronize_ownCloud_QModelIndex = {
    "column": 0,
    "container": folder_Sync_Connection_Deselect_remote_folders_QTreeWidget,
    "text": "ownCloud",
    "type": "QModelIndex",
}
loginRequiredDialog_OCC_LoginRequiredDialog = {
    "name": "LoginRequiredDialog",
    "type": "OCC::LoginRequiredDialog",
    "visible": 1,
}
loginRequiredDialog_contentWidget_QStackedWidget = {
    "name": "contentWidget",
    "type": "QStackedWidget",
    "visible": 1,
    "window": loginRequiredDialog_OCC_LoginRequiredDialog,
}
contentWidget_contentWidget_QStackedWidget = {
    "container": setupWizardWindow_contentWidget_QStackedWidget,
    "name": "contentWidget",
    "type": "QStackedWidget",
    "visible": 1,
}
add_Folder_Sync_Connection_tableView_QTableView = {
    "name": "tableView",
    "type": "QTableView",
    "visible": 1,
    "window": add_Folder_Sync_Connection_OCC_FolderWizard,
}
stack_scrollArea_QScrollArea = {
    "container": settings_stack_QStackedWidget,
    "name": "scrollArea",
    "type": "QScrollArea",
    "visible": 1,
}
stack_stackedWidget_QStackedWidget = {
    "container": settings_stack_QStackedWidget,
    "name": "stackedWidget",
    "type": "QStackedWidget",
    "visible": 1,
}
stackedWidget_quickWidget_QQuickWidget = {
    "container": stack_stackedWidget_QStackedWidget,
    "name": "quickWidget",
    "type": "QQuickWidget",
    "visible": 1,
}
quickWidget_scrollView_ScrollView = {
    "container": stackedWidget_quickWidget_QQuickWidget,
    "id": "scrollView",
    "type": "ScrollView",
    "unnamed": 1,
    "visible": True,
}
scrollView_ListView = {
    "container": quickWidget_scrollView_ScrollView,
    "type": "ListView",
    "unnamed": 1,
    "visible": True,
}
settings_dialogStack_QStackedWidget = {
    "name": "dialogStack",
    "type": "QStackedWidget",
    "visible": 1,
    "window": settings_OCC_SettingsDialog,
}
dialogStack_quickWidget_QQuickWidget = {
    "container": settings_dialogStack_QStackedWidget,
    "name": "quickWidget",
    "type": "QQuickWidget",
    "visible": 1,
}
create_Remote_Folder_QInputDialog = {
    "type": "QInputDialog",
    "unnamed": 1,
    "visible": 1,
    "windowTitle": "Create Remote Folder",
}
create_Remote_Folder_Enter_the_name_of_the_new_folder_to_be_created_below_QLabel = {
    "text": "Enter the name of the new folder to be created below '/':",
    "type": "QLabel",
    "unnamed": 1,
    "visible": 1,
    "window": create_Remote_Folder_QInputDialog,
}
groupBox_folderTreeWidget_QTreeWidget = {
    "container": add_Folder_Sync_Connection_groupBox_QGroupBox,
    "name": "folderTreeWidget",
    "type": "QTreeWidget",
    "visible": 1,
}
confirm_Folder_Sync_Connection_Removal_QMessageBox = {
    "type": "QMessageBox",
    "unnamed": 1,
    "visible": 1,
    "windowTitle": "Confirm Folder Sync Connection Removal",
}
stackedWidget_quickWidget_OCC_QmlUtils_OCQuickWidget = {
    "container": stack_stackedWidget_QStackedWidget,
    "name": "quickWidget",
    "type": "OCC::QmlUtils::OCQuickWidget",
    "visible": 1,
}
qt_tabwidget_stackedwidget_OCC_ProtocolWidget_OCC_ProtocolWidget = {
    "container": stack_qt_tabwidget_stackedwidget_QStackedWidget,
    "name": "OCC__ProtocolWidget",
    "type": "OCC::ProtocolWidget",
    "visible": 1,
}
oCC_ProtocolWidget_tableView_QTableView = {
    "container": qt_tabwidget_stackedwidget_OCC_ProtocolWidget_OCC_ProtocolWidget,
    "name": "_tableView",
    "type": "QTableView",
    "visible": 1,
}
oCC_IssuesWidget_tableView_QTableView = {
    "container": qt_tabwidget_stackedwidget_OCC_IssuesWidget_OCC_IssuesWidget,
    "name": "_tableView",
    "type": "QTableView",
    "visible": 1,
}
dialogStack_quickWidget_OCC_QmlUtils_OCQuickWidget = {
    "container": settings_dialogStack_QStackedWidget,
    "name": "quickWidget",
    "type": "OCC::QmlUtils::OCQuickWidget",
    "visible": 1,
}
contentWidget_OCC_QmlUtils_OCQuickWidget = {
    "container": contentWidget_contentWidget_QStackedWidget,
    "type": "OCC::QmlUtils::OCQuickWidget",
    "unnamed": 1,
    "visible": 1,
}
stackedWidget_Add_Folder_Sync_Connection_QGroupBox = {
    "container": stack_stackedWidget_QStackedWidget,
    "title": "Add Folder Sync Connection",
    "type": "QGroupBox",
    "unnamed": 1,
    "visible": 1,
}
stackedWidget_groupBox_QGroupBox = {
    "container": settings_stack_QStackedWidget,
    "name": "groupBox",
    "type": "QGroupBox",
    "visible": 1,
}
groupBox_OCC_QmlUtils_OCQuickWidget = {
    "container": stackedWidget_groupBox_QGroupBox,
    "type": "OCC::QmlUtils::OCQuickWidget",
    "unnamed": 1,
    "visible": 1,
}
