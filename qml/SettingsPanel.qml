import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 0

    signal closeRequested()

    // Header
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: theme.sp48
        color: theme.bgSurface2

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: theme.sp16
            anchors.rightMargin: theme.sp8

            Label {
                text: theme.iconSettings + "  Settings"
                font.pixelSize: theme.fontSubhead
                font.weight: Font.DemiBold
                color: theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 28; height: 28
                radius: theme.radiusSmall
                color: closeBtnArea.containsMouse ? theme.bgOverlay : "transparent"

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: "\u2715"
                    font.pixelSize: theme.fontBody
                    color: closeBtnArea.containsMouse ? theme.textPrimary : theme.textMuted
                    anchors.centerIn: parent

                    Behavior on color { ColorAnimation { duration: theme.animFast } }
                }

                MouseArea {
                    id: closeBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: closeRequested()
                }
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        height: 1
        color: theme.borderSubtle
    }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentHeight: settingsContent.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

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

        ColumnLayout {
            id: settingsContent
            width: parent.width
            spacing: theme.sp4

            // ─── Appearance Section ───
            Item { height: theme.sp16; Layout.fillWidth: true }

            Label {
                text: "APPEARANCE"
                font.pixelSize: theme.fontCaption
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: theme.textMuted
                Layout.leftMargin: theme.sp16
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: theme.sp12
                Layout.rightMargin: theme.sp12
                Layout.topMargin: theme.sp8
                height: theme.sp48
                radius: theme.radiusMedium
                color: theme.bgSurface2
                border.color: theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: theme.sp12
                    anchors.rightMargin: theme.sp12
                    spacing: theme.sp8

                    Label {
                        text: "Theme"
                        font.pixelSize: theme.fontSmall
                        font.weight: Font.DemiBold
                        color: theme.textSecondary
                    }

                    Item { Layout.fillWidth: true }

                    // Toggle pill
                    Rectangle {
                        width: 52
                        height: 28
                        radius: theme.radiusPill
                        color: theme.darkMode ? theme.bgOverlay : theme.accentWarm
                        border.color: theme.darkMode ? theme.border : Qt.darker(theme.accentWarm, 1.2)
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: theme.animNormal } }

                        Rectangle {
                            id: toggleKnob
                            width: 22
                            height: 22
                            radius: 11
                            color: theme.textPrimary
                            anchors.verticalCenter: parent.verticalCenter
                            x: theme.darkMode ? parent.width - width - 3 : 3

                            Behavior on x { NumberAnimation { duration: theme.animNormal; easing.type: Easing.InOutCubic } }

                            Label {
                                text: theme.darkMode ? theme.iconMoon : theme.iconSun
                                font.pixelSize: 12
                                color: theme.darkMode ? "#1e1e2e" : "#4c4f69"
                                anchors.centerIn: parent
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: theme.darkMode = !theme.darkMode
                        }
                    }

                    Label {
                        text: theme.darkMode ? "Dark" : "Light"
                        font.pixelSize: theme.fontSmall
                        color: theme.textSecondary
                        Layout.preferredWidth: 32
                    }
                }
            }

            // ─── Vault Section ───
            Item { height: theme.sp24; Layout.fillWidth: true }

            Label {
                text: "VAULT"
                font.pixelSize: theme.fontCaption
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: theme.textMuted
                Layout.leftMargin: theme.sp16
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: theme.sp12
                Layout.rightMargin: theme.sp12
                Layout.topMargin: theme.sp8
                height: vaultInfoCol.height + theme.sp24
                radius: theme.radiusMedium
                color: theme.bgSurface2
                border.color: theme.borderSubtle
                border.width: 1

                ColumnLayout {
                    id: vaultInfoCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: theme.sp12
                    spacing: theme.sp8

                    RowLayout {
                        spacing: theme.sp8
                        Label {
                            text: "Path"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.textSecondary
                            Layout.preferredWidth: 56
                        }
                        Label {
                            text: vaultManager.vaultPath
                            font.pixelSize: theme.fontSmall
                            color: theme.textPrimary
                            wrapMode: Text.WrapAnywhere
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        spacing: theme.sp8
                        Label {
                            text: "Articles"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.textSecondary
                            Layout.preferredWidth: 56
                        }
                        Label {
                            text: fileTreeModel.fileCount
                            font.pixelSize: theme.fontSmall
                            color: theme.textPrimary
                        }
                    }
                }
            }

            // ─── LLM Section ───
            Item { height: theme.sp24; Layout.fillWidth: true }

            Label {
                text: "LLM PROVIDER"
                font.pixelSize: theme.fontCaption
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: theme.textMuted
                Layout.leftMargin: theme.sp16
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: theme.sp12
                Layout.rightMargin: theme.sp12
                Layout.topMargin: theme.sp8
                height: llmCol.height + theme.sp24
                radius: theme.radiusMedium
                color: theme.bgSurface2
                border.color: theme.borderSubtle
                border.width: 1

                ColumnLayout {
                    id: llmCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: theme.sp12
                    spacing: theme.sp4

                    // Local option
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        radius: theme.radiusSmall
                        color: localRadio.checked ? theme.bgOverlay : "transparent"

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: theme.sp8
                            anchors.rightMargin: theme.sp8
                            spacing: theme.sp8

                            Rectangle {
                                width: 16; height: 16
                                radius: 8
                                border.color: localRadio.checked ? theme.accent : theme.textMuted
                                border.width: 1.5
                                color: "transparent"

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: theme.accent
                                    anchors.centerIn: parent
                                    visible: localRadio.checked
                                    scale: localRadio.checked ? 1.0 : 0.0

                                    Behavior on scale { NumberAnimation { duration: theme.animFast } }
                                }
                            }

                            Label {
                                text: "Local (llama.cpp)"
                                font.pixelSize: theme.fontSmall
                                color: theme.textPrimary
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: localRadio.checked = true
                        }

                        RadioButton {
                            id: localRadio
                            checked: true
                            visible: false
                        }
                    }

                    // Cloud option
                    Rectangle {
                        Layout.fillWidth: true
                        height: 36
                        radius: theme.radiusSmall
                        color: cloudRadio.checked ? theme.bgOverlay : "transparent"

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: theme.sp8
                            anchors.rightMargin: theme.sp8
                            spacing: theme.sp8

                            Rectangle {
                                width: 16; height: 16
                                radius: 8
                                border.color: cloudRadio.checked ? theme.accent : theme.textMuted
                                border.width: 1.5
                                color: "transparent"

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: theme.accent
                                    anchors.centerIn: parent
                                    visible: cloudRadio.checked
                                    scale: cloudRadio.checked ? 1.0 : 0.0

                                    Behavior on scale { NumberAnimation { duration: theme.animFast } }
                                }
                            }

                            Label {
                                text: "Cloud (Claude / OpenAI)"
                                font.pixelSize: theme.fontSmall
                                color: theme.textPrimary
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: cloudRadio.checked = true
                        }

                        RadioButton {
                            id: cloudRadio
                            visible: false
                        }
                    }

                    Item { height: theme.sp4; Layout.fillWidth: true }

                    Rectangle {
                        Layout.fillWidth: true
                        height: phaseLabel.height + theme.sp12
                        radius: theme.radiusSmall
                        color: theme.bgOverlay
                        opacity: 0.6

                        Label {
                            id: phaseLabel
                            text: "LLM integration available in Phase 3"
                            font.pixelSize: theme.fontCaption
                            font.italic: true
                            color: theme.accentWarm
                            anchors.centerIn: parent
                        }
                    }
                }
            }

            // ─── Data Sources Section ───
            Item { height: theme.sp24; Layout.fillWidth: true }

            Label {
                text: "DATA SOURCES"
                font.pixelSize: theme.fontCaption
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                color: theme.textMuted
                Layout.leftMargin: theme.sp16
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: theme.sp12
                Layout.rightMargin: theme.sp12
                Layout.topMargin: theme.sp8
                height: sourcesCol.height + theme.sp24
                radius: theme.radiusMedium
                color: theme.bgSurface2
                border.color: theme.borderSubtle
                border.width: 1

                ColumnLayout {
                    id: sourcesCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: theme.sp12
                    spacing: theme.sp4

                    Repeater {
                        model: [
                            { icon: theme.iconPhoto, label: "Photos (EXIF)", enabled: false },
                            { icon: theme.iconChat,  label: "iMessages (chat.db)", enabled: false },
                            { icon: theme.iconSnap,  label: "Snapchat (JSON export)", enabled: false }
                        ]

                        Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            radius: theme.radiusSmall
                            color: sourceArea.containsMouse ? theme.bgOverlay : "transparent"

                            Behavior on color { ColorAnimation { duration: theme.animFast } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: theme.sp8
                                anchors.rightMargin: theme.sp8
                                spacing: theme.sp8

                                Label {
                                    text: modelData.icon
                                    font.pixelSize: theme.fontBody
                                    color: theme.textMuted
                                    Layout.preferredWidth: 20
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Label {
                                    text: modelData.label
                                    font.pixelSize: theme.fontSmall
                                    color: theme.textSecondary
                                    Layout.fillWidth: true
                                }

                                // Status pill
                                Rectangle {
                                    width: statusText.width + theme.sp12
                                    height: 20
                                    radius: theme.radiusPill
                                    color: theme.bgOverlay

                                    Label {
                                        id: statusText
                                        text: "Phase 2"
                                        font.pixelSize: theme.fontCaption
                                        color: theme.textMuted
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            MouseArea {
                                id: sourceArea
                                anchors.fill: parent
                                hoverEnabled: true
                            }
                        }
                    }
                }
            }

            // ─── About ───
            Item { height: theme.sp32; Layout.fillWidth: true }

            Column {
                Layout.alignment: Qt.AlignHCenter
                spacing: theme.sp4

                Label {
                    text: "Arkive v0.1.0"
                    font.pixelSize: theme.fontCaption
                    color: theme.textMuted
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Label {
                    text: "Your life wiki. Local-only."
                    font.pixelSize: theme.fontCaption
                    color: theme.textMuted
                    anchors.horizontalCenter: parent.horizontalCenter
                    opacity: 0.6
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
