import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: fileBrowser

    signal fileSelected(string path)

    ListView {
        id: treeView
        anchors.fill: parent
        anchors.margins: 4
        model: fileTreeModel
        clip: true
        spacing: 1

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        delegate: ItemDelegate {
            id: delegate
            width: treeView.width
            height: 32

            required property string name
            required property string fullPath
            required property bool isDir
            required property bool isMarkdown
            required property int index

            contentItem: RowLayout {
                spacing: 8

                // Icon
                Label {
                    text: delegate.isDir ? "📁" : (delegate.isMarkdown ? "📄" : "📎")
                    font.pixelSize: 14
                    Layout.preferredWidth: 20
                }

                // File name
                Label {
                    text: delegate.name
                    font.pixelSize: 13
                    font.bold: delegate.isDir
                    color: delegate.isDir ? accentColor : textColor
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            background: Rectangle {
                color: delegate.hovered ? bgOverlay
                     : (treeView.currentIndex === delegate.index ? bgSurface : "transparent")
                radius: 4
            }

            onClicked: {
                if (!delegate.isDir && delegate.isMarkdown) {
                    treeView.currentIndex = delegate.index
                    fileBrowser.fileSelected(delegate.fullPath)
                }
            }
        }

        // Empty state
        Label {
            anchors.centerIn: parent
            text: "No files in vault"
            font.pixelSize: 13
            color: textSubtle
            visible: treeView.count === 0
        }
    }
}
