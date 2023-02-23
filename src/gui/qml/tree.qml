import QtQuick 2.15
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.4

Item {
    TreeView {
        id: view
        anchors.fill: parent
        model: ctx.model

        TableViewColumn {
            role: "display"
        }

        headerVisible: false
        rowDelegate: Rectangle {height: 100}

        itemDelegate: Item {
            id: delegate


            function sibling(column, role) {
                var sibling = styleData.index.model.sibling(styleData.row, column, styleData.index)
                return styleData.index.model.data(sibling, role)

            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Image{
                    id: image
                    Layout.maximumHeight: 64
                    Layout.maximumWidth: 64
                    Layout.alignment: Qt.AlignHCenter
                    antialiasing: true

                    //source: styleData.index.model.data(styleData.index.model.index(styleData.row, 7))
                    source: "qrc:/client/ownCloud/theme/colored/state-ok.svg"
                }

                ColumnLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        Layout.fillWidth: true
                        id: title
                        text: styleData.value
                        font.bold: true
                        font.pointSize: 15
                        elide: Text.ElideLeft
                        verticalAlignment: Text.AlignHCenter
                    }

                    Text {
                        Layout.fillWidth: true
                        id: subtitle
                        text: sibling(1, Qt.DisplayRole)
                        elide: Text.ElideLeft
                        verticalAlignment: Text.AlignHCenter
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        Layout.maximumHeight: 10
                        value: 0.5
                    }
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (styleData.hasChildren) {
                        if(!styleData.isExpanded)
                            view.expand(styleData.index)
                        else
                            view.collapse(styleData.index)

                    }
                }
            }



        }
    }
}
