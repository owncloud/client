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
    id: navigationBar
    readonly property Navigation navigation: ocContext

    // Connections {
    //     target: ocParentWidget

    //     function onFocusFirst() {
    //         if (accountButtons.count === 0) {
    //             addAccountButton.forceActiveFocus(Qt.TabFocusReason);
    //         } else {
    //             accountButtons.itemAt(0).forceActiveFocus(Qt.TabFocusReason);
    //         }
    //     }

    //     function onFocusLast() {
    //         quitButton.forceActiveFocus(Qt.TabFocusReason);
    //     }
    // }

    RowLayout {
        anchors.fill: parent

        Item {
            Layout.fillWidth: true
        }

        Repeater {
            id: statesRepeater

            model: navigation.states

            delegate: RadioButton {
                property int wizardState: modelData
                Accessible.role: Accessible.PageTab
                checked: navigation.currentState === wizardState
                text: navigation.stateDisplayName(wizardState)
                onClicked: {
                    // settingsDialog.currentAccount = accountState.account;
                }
            }
        }
      }

    Item {
        Layout.fillWidth: true
    }
}
