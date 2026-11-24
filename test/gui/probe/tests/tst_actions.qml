// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

import QtQuick

Item {
    width: 300
    height: 200

    Rectangle {
        objectName: "tapTarget"
        x: 20
        y: 20
        width: 100
        height: 40
        color: "lightgray"
        property int clicks: 0

        MouseArea {
            anchors.fill: parent
            onClicked: parent.clicks += 1
        }
    }

    TextInput {
        objectName: "quickInput"
        x: 20
        y: 100
        width: 200
        height: 30
    }
}
