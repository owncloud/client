/*
 * Copyright (C) by Hannah von Reth <hannah.vonreth@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.ownCloud.gui 1.0
import org.ownCloud.libsync 1.0
import org.ownCloud.resources 1.0

Pane {
    id: folderSyncPanel
    // TODO: not cool
    readonly property real normalSize: 170
    // todo: #16
    readonly property AccountSettings accountSettings: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Folder Sync")

    Connections {
        target: widget

        function onFocusFirst() {
            listView.forceActiveFocus(Qt.TabFocusReason);
        }

        function onFocusLast() {
            if (addSyncButton.enabled) {
                addSyncButton.forceActiveFocus(Qt.TabFocusReason);
            } else {
                addSyncButton.nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocusReason);
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        ScrollView {
            id: scrollView
            Layout.fillHeight: true
            Layout.fillWidth: true

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

            clip: true

            ListView {
                id: listView
                anchors.fill: parent
                focus: true
                boundsBehavior: Flickable.StopAtBounds

                model: accountSettings.model
                spacing: 20

                onCurrentItemChanged: {
                    if (currentItem) {
                        currentItem.forceActiveFocus(Qt.TabFocusReason);
                    }
                }

                delegate: FocusScope {
                    id: folderDelegate

                    implicitHeight: normalSize
                    width: ListView.view.width - scrollView.ScrollBar.vertical.width - 10

                    required property string displayName
                    required property var errorMsg
                    required property Folder folder
                    required property string itemText
                    required property string overallText
                    required property double progress
                    required property string quota
                    required property string accessibleDescription
                    required property string statusIcon
                    required property string subtitle
                    // model index
                    required property int index

                    Pane {
                        id: delegatePane
                        anchors.fill: parent

                        Accessible.name: folderDelegate.accessibleDescription
                        Accessible.role: Accessible.ListItem

                        clip: true
                        activeFocusOnTab: true
                        focus: true

                        background: Rectangle {
                            color: scrollView.palette.base
                            border.width: delegatePane.visualFocus || folderDelegate.ListView.isCurrentItem ? 2 : 0
                            border.color: delegatePane.visualFocus || folderDelegate.ListView.isCurrentItem ? scrollView.palette.highlight : scrollView.palette.base
                        }

                        Keys.onBacktabPressed: {
                            widget.parentFocusWidget.focusPrevious();
                        }
                        Keys.onTabPressed: {
                            moreButton.forceActiveFocus(Qt.TabFocusReason);
                        }

                        MouseArea {
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            anchors.fill: parent

                            onClicked: mouse => {
                                if (mouse.button === Qt.RightButton) {
                                    accountSettings.slotCustomContextMenuRequested(folder);
                                } else {
                                    folderDelegate.ListView.view.currentIndex = folderDelegate.index;
                                    folderDelegate.forceActiveFocus(Qt.TabFocusReason);
                                }
                            }
                        }
                        RowLayout {
                            anchors.fill: parent
                            spacing: 10

                            SpaceDelegate {
                                Layout.fillHeight: true
                                Layout.fillWidth: true
                                description: folderDelegate.subtitle
                                imageSource: folderDelegate.folder.space ? folderDelegate.folder.space.image.qmlImageUrl : QMLResources.resourcePath("core", "folder-sync-small", enabled)
                                statusSource: QMLResources.resourcePath("core", statusIcon, enabled)
                                title: displayName

                                // we will either display quota or overallText
                                Label {
                                    Accessible.ignored: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                    text: folderDelegate.quota
                                    visible: folderDelegate.quota && !folderDelegate.overallText
                                }
                                Item {
                                    Accessible.ignored: true
                                    Layout.fillWidth: true
                                    // ensure the progress bar always consumes its space
                                    Layout.preferredHeight: 10

                                    ProgressBar {
                                        anchors.fill: parent
                                        value: folderDelegate.progress
                                        visible: folderDelegate.overallText || folderDelegate.itemText
                                    }
                                }
                                Label {
                                    Accessible.ignored: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                    text: folderDelegate.overallText
                                }
                                Label {
                                    Accessible.ignored: true
                                    Layout.fillWidth: true
                                    elide: Text.ElideMiddle
                                    text: folderDelegate.itemText
                                }
                                FolderError {
                                    Accessible.ignored: true
                                    Layout.fillWidth: true
                                    errorMessages: folderDelegate.errorMsg

                                    onCollapsedChanged: {
                                        if (!collapsed) {
                                            // TODO: not cool
                                            folderDelegate.implicitHeight = normalSize + implicitHeight + 10;
                                        } else {
                                            folderDelegate.implicitHeight = normalSize;
                                        }
                                    }
                                }
                            }
                            Button {
                                id: moreButton

                                Accessible.name: delegatePane.Accessible.name
                                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                                Layout.maximumHeight: 30
                                display: AbstractButton.IconOnly
                                icon.source: QMLResources.resourcePath("core", "more", enabled)
                                // this should have no effect, but without it the highlight is not displayed in Qt 6.7 on Windows
                                palette.highlight: folderSyncPanel.palette.higlight

                                Keys.onTabPressed: {
                                    if (addSyncButton.enabled) {
                                        addSyncButton.forceActiveFocus(Qt.TabFocusReason);
                                    } else {
                                        widget.parentFocusWidget.focusNext();
                                    }
                                }

                                Keys.onBacktabPressed: {
                                    parent.forceActiveFocus(Qt.TabFocusReason);
                                }

                                onClicked: {
                                    accountSettings.slotCustomContextMenuRequested(folder);
                                }

                                // select the list item the button belongs to
                                onFocusChanged: {
                                    if (moreButton.focusReason == Qt.TabFocusReason || moreButton.focusReason == Qt.BacktabFocusReason) {
                                        folderDelegate.ListView.view.currentIndex = folderDelegate.index;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true

            Button {
                id: addSyncButton
                text: accountSettings.accountState.supportsSpaces && !Theme.spacesAreCalledFolders ? qsTr("Add Space") : qsTr("Add Folder")
                // this should have no effect, but without it the highlight is not displayed in Qt 6.7 on Windows
                palette.highlight: folderSyncPanel.palette.highlight

                onClicked: {
                    accountSettings.slotAddFolder();
                }
                //todo: #16
                enabled: (accountSettings.accountState.state === AccountState.Connected) && (accountSettings.unsyncedSpaces || !accountSettings.accountState.supportsSpaces)

                // with Spaces Theme.singleSyncFolder hast no effect
                visible: accountSettings.accountState.supportsSpaces ? !Theme.syncNewlyDiscoveredSpaces : (listView.count === 0 || !Theme.singleSyncFolder)

                Keys.onBacktabPressed: {
                    listView.currentItem.forceActiveFocus(Qt.TabFocusReason);
                }

                Keys.onTabPressed: {
                    widget.parentFocusWidget.focusNext();
                }
            }
            Item {
                // spacer
                Layout.fillWidth: true
            }
            Label {
                text: (Theme.spacesAreCalledFolders ? qsTr("You are synchronizing %1 out of %n folder(s)", "", accountSettings.syncedSpaces + accountSettings.unsyncedSpaces) : qsTr("You are synchronizing %1 out of %n space(s)", "", accountSettings.syncedSpaces + accountSettings.unsyncedSpaces)).arg(accountSettings.syncedSpaces)
                visible: accountSettings.accountState.supportsSpaces && accountSettings.accountState.state === AccountState.Connected
            }
        }
    }
}
