/*
 * SPDX-FileCopyrightText: 2012-2016 ownCloud Inc.
 * SPDX-FileCopyrightText: 2016-2022 ownCloud GmbH
 * SPDX-FileCopyrightText: 2023 ownCloud GmbH - A Kiteworks Company
 * SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ownCloud.gui 1.0
import org.ownCloud.libsync 1.0
import org.ownCloud.resources 1.0

Pane {
    id: bar
    readonly property SettingsDialog settingsDialog: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    Accessible.name: qsTr("Navigation bar")

    Component.onCompleted: {
        if ('popupType' in ToolTip.toolTip) {
            ToolTip.toolTip.popupType = Popup.Native;
        }
    }

    Connections {
        target: widget

        function onFocusFirst() {
            if (accountButtons.count === 0) {
                addAccountButton.forceActiveFocus(Qt.TabFocusReason);
            } else {
                accountButtons.itemAt(0).forceActiveFocus(Qt.TabFocusReason);
            }
        }

        function onFocusLast() {
            quitButton.forceActiveFocus(Qt.TabFocusReason);
        }
    }

    RowLayout {
        anchors.fill: parent

        // don't modify the enabled state directly as it messes with the palette in Qt 6.7.2
        opacity: widget.enabled ? 1.0 : 0.5

        Repeater {
            id: accountButtons

            model: AccountManager.accounts

            delegate: AccountButton {
                property AccountState accountState: modelData

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumWidth: widthHint
                Accessible.role: Accessible.PageTab
                checked: settingsDialog.currentAccount === accountState.account
                icon.source: accountState.account.hasAvatar ? QMLResources.resourcePath2("avatar", accountState.account.uid, enabled) : undefined
                icon.cache: false
                altText: accountState.account.initials
                text: accountState.account.hostName
                gradient: accountState.account.avatarGradient
                solidColor: Theme.avatarColor
                solidColorChecked: Theme.avatarColorChecked

                hoverEnabled: true
                ToolTip.visible: hovered
                ToolTip.text: accountState.account.davDisplayName + "\n" + accountState.account.url
                ToolTip.delay: 500

                Accessible.name: accountState.account.displayNameWithHost

                Connections {
                    target: accountState.account
                    function onAvatarChanged() {
                        icon.source = undefined;
                        if (accountState.account.hasAvatar) {
                            icon.source = QMLResources.resourcePath2("avatar", accountState.account.uid, enabled);
                        }
                    }
                }

                Keys.onBacktabPressed: event => {
                    if (index === 0) {
                        // We're the first button, handle the back-tab
                        widget.parentFocusWidget.focusPrevious();
                    } else {
                        event.accepted = false;
                    }
                }
                onClicked: {
                    settingsDialog.currentAccount = accountState.account;
                }
            }
        }
        AccountButton {
            id: addAccountButton

            Layout.fillHeight: true
            Layout.maximumWidth: widthHint
            icon.source: QMLResources.resourcePath("core", "plus-solid", enabled)
            text: qsTr("Add Account")
            visible: Theme.multiAccount || AccountManager.accounts.length === 0

            Keys.onBacktabPressed: event => {
                // If there are no account buttons, we're the first button, so handle the back-tab
                if (accountButtons.count === 0) {
                    widget.parentFocusWidget.focusPrevious();
                } else {
                    event.accepted = false;
                }
            }
            onClicked: {
                settingsDialog.createNewAccount();
            }
        }
        Item {
            // spacer
            Layout.fillWidth: true
        }
        AccountButton {
            id: logButton

            Layout.fillHeight: true
            Layout.maximumWidth: widthHint
            Accessible.role: Accessible.PageTab
            checked: settingsDialog.currentPage === SettingsDialog.Activity
            icon.source: QMLResources.resourcePath("core", "activity", enabled)
            text: qsTr("Activity")

            onClicked: {
                settingsDialog.currentPage = SettingsDialog.Activity;
            }
        }
        AccountButton {
            id: settingsButton

            Layout.fillHeight: true
            Layout.maximumWidth: widthHint
            Accessible.role: Accessible.PageTab
            checked: settingsDialog.currentPage === SettingsDialog.Settings
            icon.source: QMLResources.resourcePath("core", "settings", enabled)
            text: qsTr("Settings")

            onClicked: {
                settingsDialog.currentPage = SettingsDialog.Settings;
            }
        }
        Repeater {
            // branded buttons with a URL
            model: Theme.urlButtons

            delegate: AccountButton {
                property urlbuttondata urlButton: modelData

                Layout.fillHeight: true
                Layout.maximumWidth: widthHint
                icon.source: QMLResources.resourcePath("universal", urlButton.icon, enabled)
                text: urlButton.name

                onClicked: {
                    Qt.openUrlExternally(urlButton.url);
                }
            }
        }
        AccountButton {
            id: quitButton

            Layout.fillHeight: true
            Layout.maximumWidth: widthHint
            icon.source: QMLResources.resourcePath("core", "quit", enabled)
            text: qsTr("Quit")

            Keys.onTabPressed: {
                widget.parentFocusWidget.focusNext();
            }
            onClicked: {
                Qt.quit();
            }
        }
    }
}
