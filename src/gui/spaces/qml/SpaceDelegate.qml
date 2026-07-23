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

Item {
    id: spaceDelegate
    property alias title: title.text
    property alias description: description.text
    property alias descriptionWrapMode: description.wrapMode
    property alias imageSource: image.source
    property alias statusSource: statusIcon.source

    default property alias content: colLayout.data

    ColumnLayout {
        anchors.fill: parent
        spacing: 10

        RowLayout {
            Layout.alignment: Qt.AlignTop
            Layout.fillWidth: true

            Pane {
                Accessible.ignored: true
                Layout.preferredHeight: normalSize - 20
                Layout.preferredWidth: normalSize - 20
                Layout.alignment: Qt.AlignTop
                background: Rectangle {
                    color: spaceDelegate.palette.alternateBase
                }
                Image {
                    id: image
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: width
                    sourceSize.height: height
                }
            }
            ColumnLayout {
                id: colLayout
                spacing: 6
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                Layout.fillWidth: true

                RowLayout {
                    Layout.fillWidth: true
                    Image {
                        id: statusIcon
                        Layout.preferredHeight: 16
                        Layout.preferredWidth: 16
                        visible: statusSource
                        sourceSize.width: width
                        sourceSize.height: height
                    }
                    Label {
                        id: title
                        Accessible.ignored: true
                        Layout.fillWidth: true
                        font.bold: true
                        font.pointSize: 15
                        elide: Text.ElideRight
                    }
                }
                Label {
                    id: description
                    Accessible.ignored: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
            }
        }
    }
}
