import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: fileBrowser

    signal fileSelected(string path)

    ListView {
        id: treeView
        anchors.fill: parent
        anchors.margins: theme.sp4
        model: fileTreeModel
        clip: true
        spacing: theme.sp2

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

        delegate: Item {
            id: delegate
            width: treeView.width
            height: 32

            required property string name
            required property string fullPath
            required property bool isDir
            required property bool isMarkdown
            required property int index

            property bool isSelected: treeView.currentIndex === delegate.index
            property bool isClickable: !delegate.isDir && delegate.isMarkdown

            Rectangle {
                id: delegateBg
                anchors.fill: parent
                anchors.leftMargin: theme.sp2
                anchors.rightMargin: theme.sp2
                radius: theme.radiusSmall
                color: delegate.isSelected ? theme.bgOverlay
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
                    visible: delegate.isSelected
                    opacity: delegate.isSelected ? 1.0 : 0.0

                    Behavior on opacity { NumberAnimation { duration: theme.animNormal } }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: theme.sp12
                    anchors.rightMargin: theme.sp8
                    spacing: theme.sp8

                    // Icon
                    Label {
                        text: delegate.isDir ? theme.iconFolder
                            : delegate.isMarkdown ? theme.iconMarkdown
                            : theme.iconFile
                        font.pixelSize: theme.fontSmall
                        color: delegate.isDir ? theme.accentWarm
                             : delegate.isSelected ? theme.accent
                             : delegate.isMarkdown ? theme.textSecondary
                             : theme.textMuted
                        Layout.preferredWidth: 16
                        horizontalAlignment: Text.AlignHCenter

                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                    }

                    // File name
                    Label {
                        text: delegate.name
                        font.pixelSize: theme.fontSmall
                        font.weight: delegate.isDir ? Font.DemiBold : Font.Normal
                        color: delegate.isSelected ? theme.textPrimary
                             : delegate.isDir ? theme.textPrimary
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
                cursorShape: delegate.isClickable ? Qt.PointingHandCursor : Qt.ArrowCursor

                onClicked: {
                    if (delegate.isClickable) {
                        treeView.currentIndex = delegate.index
                        fileBrowser.fileSelected(delegate.fullPath)
                    }
                }
            }
        }

        // Empty state
        Column {
            anchors.centerIn: parent
            spacing: theme.sp8
            visible: treeView.count === 0

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
