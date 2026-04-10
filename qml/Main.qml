import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1200
    height: 800
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: "Arkive — Your Life Wiki"
    color: "#1e1e2e"

    // Color palette (Catppuccin Mocha inspired)
    readonly property color bgBase: "#1e1e2e"
    readonly property color bgSurface: "#313244"
    readonly property color bgOverlay: "#45475a"
    readonly property color textColor: "#cdd6f4"
    readonly property color textSubtle: "#a6adc8"
    readonly property color accentColor: "#89b4fa"
    readonly property color accentSecondary: "#f5c2e7"
    readonly property color borderColor: "#585b70"

    header: ToolBar {
        background: Rectangle { color: root.bgSurface }
        height: 48

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Label {
                text: "⬡ Arkive"
                font.pixelSize: 18
                font.bold: true
                color: root.accentColor
            }

            Item { Layout.fillWidth: true }

            Label {
                text: fileTreeModel.fileCount + " articles"
                font.pixelSize: 13
                color: root.textSubtle
            }

            ToolButton {
                text: "⚙"
                font.pixelSize: 18
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: root.textSubtle
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: settingsDrawer.open()
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // Left sidebar — File Browser
        Rectangle {
            SplitView.preferredWidth: 280
            SplitView.minimumWidth: 200
            SplitView.maximumWidth: 500
            color: root.bgSurface

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Sidebar header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    color: root.bgOverlay

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Label {
                            text: "Vault"
                            font.pixelSize: 14
                            font.bold: true
                            color: root.textColor
                        }

                        Item { Layout.fillWidth: true }

                        ToolButton {
                            text: "↻"
                            font.pixelSize: 14
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: root.textSubtle
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: fileTreeModel.refresh()
                            ToolTip.text: "Refresh"
                            ToolTip.visible: hovered
                        }
                    }
                }

                // File browser
                FileBrowser {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onFileSelected: function(path) {
                        articleModel.loadFromFile(path)
                    }
                }
            }
        }

        // Right content area — Article View
        Rectangle {
            SplitView.fillWidth: true
            color: root.bgBase

            ArticleView {
                anchors.fill: parent
            }
        }
    }

    // Empty state overlay
    Rectangle {
        anchors.centerIn: parent
        width: 400
        height: 300
        color: "transparent"
        visible: !articleModel.loaded && fileTreeModel.fileCount === 0

        Column {
            anchors.centerIn: parent
            spacing: 16

            Label {
                text: "⬡"
                font.pixelSize: 64
                color: root.accentColor
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: "Welcome to Arkive"
                font.pixelSize: 24
                font.bold: true
                color: root.textColor
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: "Your vault is empty. Import data to get started."
                font.pixelSize: 14
                color: root.textSubtle
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Button {
                text: "Import Data..."
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 14
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: root.bgBase
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.lighter(root.accentColor, 1.1) : root.accentColor
                    radius: 6
                    implicitWidth: 140
                    implicitHeight: 36
                }
            }
        }
    }

    // Settings drawer
    Drawer {
        id: settingsDrawer
        width: 350
        height: root.height
        edge: Qt.RightEdge

        background: Rectangle { color: root.bgSurface }

        SettingsPanel {
            anchors.fill: parent
        }
    }
}
