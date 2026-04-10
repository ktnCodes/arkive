import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 0

    // Header
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 48
        color: bgOverlay

        Label {
            text: "Settings"
            font.pixelSize: 16
            font.bold: true
            color: textColor
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 16
        }
    }

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            width: parent.width
            spacing: 24
            anchors.margins: 16

            // Vault info section
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 16

                label: Label {
                    text: "Vault"
                    font.pixelSize: 14
                    font.bold: true
                    color: textColor
                }

                background: Rectangle {
                    color: bgSurface
                    border.color: borderColor
                    radius: 8
                    y: parent.label.height / 2
                    height: parent.height - y
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label {
                        text: "Path: " + vaultManager.vaultPath
                        font.pixelSize: 12
                        color: textSubtle
                        wrapMode: Text.WrapAnywhere
                    }

                    Label {
                        text: "Articles: " + fileTreeModel.fileCount
                        font.pixelSize: 12
                        color: textSubtle
                    }
                }
            }

            // LLM Settings section (placeholder for Phase 3)
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 16

                label: Label {
                    text: "LLM Provider"
                    font.pixelSize: 14
                    font.bold: true
                    color: textColor
                }

                background: Rectangle {
                    color: bgSurface
                    border.color: borderColor
                    radius: 8
                    y: parent.label.height / 2
                    height: parent.height - y
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 12

                    RowLayout {
                        spacing: 8
                        RadioButton {
                            id: localRadio
                            text: "Local (llama.cpp)"
                            checked: true
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 13
                                color: textColor
                                leftPadding: parent.indicator.width + 8
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    RowLayout {
                        spacing: 8
                        RadioButton {
                            text: "Cloud (Claude / OpenAI)"
                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 13
                                color: textColor
                                leftPadding: parent.indicator.width + 8
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    Label {
                        text: "⚠ LLM integration available in Phase 3"
                        font.pixelSize: 11
                        font.italic: true
                        color: textSubtle
                    }
                }
            }

            // Import section (placeholder for Phase 2)
            GroupBox {
                Layout.fillWidth: true
                Layout.margins: 16

                label: Label {
                    text: "Data Sources"
                    font.pixelSize: 14
                    font.bold: true
                    color: textColor
                }

                background: Rectangle {
                    color: bgSurface
                    border.color: borderColor
                    radius: 8
                    y: parent.label.height / 2
                    height: parent.height - y
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    Label { text: "📷 Photos (EXIF)"; font.pixelSize: 13; color: textSubtle }
                    Label { text: "💬 iMessages (chat.db)"; font.pixelSize: 13; color: textSubtle }
                    Label { text: "👻 Snapchat (JSON export)"; font.pixelSize: 13; color: textSubtle }

                    Label {
                        text: "⚠ Import available in Phase 2"
                        font.pixelSize: 11
                        font.italic: true
                        color: textSubtle
                    }
                }
            }

            // About
            Label {
                text: "Arkive v0.1.0"
                font.pixelSize: 11
                color: textSubtle
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 16
            }

            Label {
                text: "Your life wiki. Local-only."
                font.pixelSize: 11
                color: textSubtle
                Layout.alignment: Qt.AlignHCenter
            }

            Item { Layout.fillHeight: true }
        }
    }
}
