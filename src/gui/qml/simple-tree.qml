import QtQuick 2.15
import QtQuick.Controls 1.4

Item {
    TreeView {
        id: view
        anchors.fill: parent
        model: ctx.model

        TableViewColumn {
            title: "Folder"
            role: "display"
        }




        itemDelegate: Item {
            id: delegate

            CheckBox {
                id: box
                checkedState: styleData.index.model.data(styleData.index, Qt.CheckStateRole)
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        console.log(box.checkedState)
                        styleData.index.model.setData(styleData.index, box.checkedState == Qt.Unchecked ? Qt.Checked : Qt.Unchecked, Qt.CheckStateRole)
                    }
                }
            }


            Text {
                id: title

                anchors.left: box.right

                text: {
                    return (styleData.hasChildren ? "▶️ " : "") + styleData.value
                }
                elide: Text.ElideLeft
                verticalAlignment: Text.AlignHCenter
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (styleData.hasChildren) {
                            console.log(styleData.row + (styleData.isExpanded ? "true" : "false"))
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
}
