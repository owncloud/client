// SPDX-FileCopyrightText: 2026 ownCloud GmbH - A Kiteworks Company and ownCloud contributors
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls

Item {
    id: page
    width: 300
    height: 300

    ScrollView {
        id: scrollView
        anchors.fill: parent

        Column {
            TextField {
                id: userNameField
                width: 200
            }
            Button {
                id: loginButton
                text: "Log in"
                // an object declaring a property of its own gets a derived meta-object
                property int clicks: 0
            }
            Label {
                objectName: "aliceLabel"
                text: "alice"
            }
            Text {
                id: hiddenText
                text: "hidden"
                visible: false
            }
            Button {
                id: disabledButton
                text: "Disabled"
                enabled: false
            }
            ListView {
                id: spaces
                width: 200
                height: 60
                model: 2
                delegate: Label {
                    text: "space " + index
                }
            }
        }
    }
}
