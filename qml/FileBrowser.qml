import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: fileBrowser

    signal fileSelected(string path)

    TreeView {
        id: treeView
        anchors.fill: parent
        anchors.margins: theme.sp4
        model: fileTreeModel
        clip: true

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 4
                radius: 2
                color: parent.hovered ? theme.textMuted : theme.bgOverlay
                opacity: parent.active ? 1.0 : 0.0

                Behavior on color { ColorAnimation { duration: theme.animFast } }
                Behavior on opacity { NumberAnimation { duration: theme.animNormal } }
            }
        }

        // Start with top-level folders expanded
        onModelChanged: {
            expandRecursively(0, 0)
        }

        selectionModel: ItemSelectionModel {
            id: selModel
            model: fileTreeModel
        }

        delegate: Item {
            id: delegate
            implicitWidth: treeView.width
            implicitHeight: 32

            required property TreeView treeView
            required property bool isTreeNode
            required property bool expanded
            required property bool hasChildren
            required property int depth
            required property int row
            required property int column
            required property bool current

            property string itemName: model.name ?? ""
            property string itemFullPath: model.fullPath ?? ""
            property bool itemIsDir: model.isDir ?? false
            property bool itemIsMarkdown: model.isMarkdown ?? false
            property bool isClickable: !delegate.itemIsDir && delegate.itemIsMarkdown

            Rectangle {
                id: delegateBg
                anchors.fill: parent
                anchors.leftMargin: theme.sp2
                anchors.rightMargin: theme.sp2
                radius: theme.radiusSmall
                color: delegate.current ? theme.bgOverlay
                     : delegateArea.containsMouse ? theme.bgSurface2
                     : "transparent"

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                // Selection accent bar
                Rectangle {
                    id: selectionBar
                    width: 3
                    height: parent.height - theme.sp8
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    radius: 2
                    color: theme.accent
                    visible: delegate.current && !delegate.itemIsDir
                    opacity: delegate.current ? 1.0 : 0.0

                    Behavior on opacity { NumberAnimation { duration: theme.animNormal } }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: theme.sp12 + (delegate.depth * theme.sp16)
                    anchors.rightMargin: theme.sp8
                    spacing: theme.sp8

                    // Icon
                    Label {
                        text: delegate.itemIsDir
                            ? (delegate.expanded ? theme.iconFolderOpen : theme.iconFolder)
                            : delegate.itemIsMarkdown ? theme.iconMarkdown
                            : theme.iconFile
                        font.pixelSize: theme.fontSmall
                        color: delegate.itemIsDir ? theme.accentWarm
                             : delegate.current ? theme.accent
                             : delegate.itemIsMarkdown ? theme.textSecondary
                             : theme.textMuted
                        Layout.preferredWidth: 16
                        horizontalAlignment: Text.AlignHCenter

                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }

                    // File name
                    Label {
                        text: delegate.itemName
                        font.pixelSize: theme.fontSmall
                        font.weight: delegate.itemIsDir ? Font.DemiBold : Font.Normal
                        color: delegate.current ? theme.textPrimary
                             : delegate.itemIsDir ? theme.textPrimary
                             : delegateArea.containsMouse ? theme.textPrimary
                             : theme.textSecondary
                        elide: Text.ElideRight
                        Layout.fillWidth: true

                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }
                }
            }

            MouseArea {
                id: delegateArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: (delegate.isClickable || delegate.itemIsDir) ? Qt.PointingHandCursor : Qt.ArrowCursor

                onClicked: {
                    if (delegate.itemIsDir) {
                        console.info("UI: Folder toggled:", delegate.itemName)
                        treeView.toggleExpanded(delegate.row)
                    } else if (delegate.isClickable) {
                        console.info("UI: Article selected:", delegate.itemFullPath)
                        treeView.selectionModel.setCurrentIndex(
                            treeView.index(delegate.row, 0),
                            ItemSelectionModel.ClearAndSelect)
                        fileBrowser.fileSelected(delegate.itemFullPath)
                    }
                }
            }
        }

        // Empty state
        Column {
            anchors.centerIn: parent
            spacing: theme.sp8
            visible: treeView.rows === 0

            Label {
                text: theme.iconFolder
                font.pixelSize: theme.fontDisplay
                color: theme.textMuted
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.5
            }

            Label {
                text: "No files in vault"
                font.pixelSize: theme.fontSmall
                color: theme.textMuted
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }
    }
}
