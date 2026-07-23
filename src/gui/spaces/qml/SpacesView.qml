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

Pane {
    id: spacesView
    // TODO: not cool
    readonly property real normalSize: 170

    readonly property SpacesBrowser spacesBrowser: ocContext
    readonly property OCQuickWidget widget: ocQuickWidget

    Accessible.role: Accessible.List
    Accessible.name: qsTr("Spaces")

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn

        Connections {
            target: widget

            function onFocusFirst() {
                listView.forceActiveFocus(Qt.TabFocusReason);
            }

            function onFocusLast() {
                listView.forceActiveFocus(Qt.TabFocusReason);
            }
        }

        ListView {
            id: listView
            anchors.fill: parent
            spacing: 20
            focus: true
            boundsBehavior: Flickable.StopAtBounds

            model: spacesBrowser.model

            Component.onCompleted: {
                // clear the selection delayed, else the palette is messed up
                currentIndex = -1;
            }

            onCurrentItemChanged: {
                if (currentItem) {
                    spacesBrowser.currentSpace = currentItem.space;
                    listView.currentItem.forceActiveFocus(Qt.TabFocusReason);
                } else {
                    // clear the selected item
                    spacesBrowser.currentSpace = null;
                }
            }

            delegate: FocusScope {
                id: spaceDelegate
                required property string name
                required property string subtitle
                required property string accessibleDescription
                required property Space space

                required property int index

                width: ListView.view.width - scrollView.ScrollBar.vertical.width - 10

                implicitHeight: normalSize

                Pane {
                    id: delegatePane

                    anchors.fill: parent

                    Accessible.name: spaceDelegate.accessibleDescription
                    Accessible.role: Accessible.ListItem
                    Accessible.selectable: true
                    Accessible.selected: space === spacesBrowser.currentSpace

                    clip: true

                    activeFocusOnTab: true
                    focus: true

                    Keys.onBacktabPressed: {
                        widget.parentFocusWidget.focusPrevious();
                    }

                    Keys.onTabPressed: {
                        widget.parentFocusWidget.focusNext();
                    }

                    background: Rectangle {
                        color: spaceDelegate.ListView.isCurrentItem ? scrollView.palette.highlight : scrollView.palette.base
                    }

                    RowLayout {
                        anchors.fill: parent

                        spacing: 10
                        SpaceDelegate {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            title: spaceDelegate.name
                            description: spaceDelegate.subtitle
                            imageSource: spaceDelegate.space.image.qmlImageUrl
                            descriptionWrapMode: Label.WordWrap
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            spaceDelegate.ListView.view.currentIndex = spaceDelegate.index;
                        }
                    }
                }
            }
        }
    }
}
