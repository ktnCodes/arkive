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
    color: theme.bgBase

    // Theme instance — accessible from all child QML files via scope chain
    Theme { id: theme }

    header: ToolBar {
        background: Rectangle { color: theme.bgSurface }
        height: theme.sp48

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: theme.sp16
            anchors.rightMargin: theme.sp16
            spacing: theme.sp12

            // Logo
            Label {
                text: theme.iconHex
                font.pixelSize: theme.fontTitle
                color: theme.accent
            }

            Label {
                text: "Arkive"
                font.pixelSize: 18
                font.weight: Font.Bold
                color: theme.textPrimary
                Layout.rightMargin: theme.sp8
            }

            // Search bar
            Rectangle {
                Layout.fillWidth: true
                Layout.maximumWidth: 400
                height: 32
                radius: theme.radiusPill
                color: theme.bgOverlay
                border.color: searchField.activeFocus ? theme.accent : "transparent"
                border.width: 1

                Behavior on border.color { ColorAnimation { duration: theme.animFast } }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: theme.sp12
                    anchors.rightMargin: theme.sp12
                    spacing: theme.sp8

                    Label {
                        text: theme.iconSearch
                        font.pixelSize: theme.fontBody
                        color: theme.textMuted
                    }

                    TextInput {
                        id: searchField
                        Layout.fillWidth: true
                        font.pixelSize: theme.fontSmall
                        color: theme.textPrimary
                        clip: true
                        verticalAlignment: Text.AlignVCenter

                        Text {
                            anchors.fill: parent
                            text: "Search vault..."
                            font: parent.font
                            color: theme.textMuted
                            verticalAlignment: Text.AlignVCenter
                            visible: !parent.text && !parent.activeFocus
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Article count
            Rectangle {
                height: 24
                width: articleCountLabel.width + theme.sp16
                radius: theme.radiusPill
                color: theme.bgOverlay

                Label {
                    id: articleCountLabel
                    text: fileTreeModel.fileCount + " articles"
                    font.pixelSize: theme.fontSmall
                    color: theme.textSecondary
                    anchors.centerIn: parent
                }
            }

            // Import button
            Rectangle {
                width: 32
                height: 32
                radius: theme.radiusMedium
                color: importBtn.containsMouse
                    ? (root.importPanelVisible ? theme.accent : theme.bgOverlay)
                    : (root.importPanelVisible ? Qt.alpha(theme.accent, 0.15) : "transparent")

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: theme.iconImport
                    font.pixelSize: theme.fontSubhead
                    color: root.importPanelVisible ? theme.accent
                         : importBtn.containsMouse ? theme.textPrimary : theme.textSecondary
                    anchors.centerIn: parent

                    Behavior on color { ColorAnimation { duration: theme.animFast } }
                }

                MouseArea {
                    id: importBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.info("UI: Import panel toggled", root.importPanelVisible ? "closed" : "opened")
                        root.importPanelVisible = !root.importPanelVisible
                        if (root.importPanelVisible) root.skillPanelVisible = false
                    }

                    ToolTip.text: "Import data"
                    ToolTip.visible: importBtn.containsMouse
                    ToolTip.delay: 600
                }
            }

            // Wiki Tools button
            Rectangle {
                width: 32
                height: 32
                radius: theme.radiusMedium
                color: skillBtn.containsMouse
                    ? (root.skillPanelVisible ? theme.accent : theme.bgOverlay)
                    : (root.skillPanelVisible ? Qt.alpha(theme.accent, 0.15) : "transparent")

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: "⬡"
                    font.pixelSize: theme.fontSubhead
                    color: root.skillPanelVisible ? theme.accent
                         : skillBtn.containsMouse ? theme.textPrimary : theme.textSecondary
                    anchors.centerIn: parent

                    Behavior on color { ColorAnimation { duration: theme.animFast } }
                }

                MouseArea {
                    id: skillBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.info("UI: Skill panel toggled", root.skillPanelVisible ? "closed" : "opened")
                        root.skillPanelVisible = !root.skillPanelVisible
                        if (root.skillPanelVisible) root.importPanelVisible = false
                    }

                    ToolTip.text: "Wiki tools"
                    ToolTip.visible: skillBtn.containsMouse
                    ToolTip.delay: 600
                }
            }

            // Dark/Light toggle
            Rectangle {
                width: 32
                height: 32
                radius: theme.radiusMedium
                color: themeToggleBtn.hovered ? theme.bgOverlay : "transparent"

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: theme.darkMode ? theme.iconSun : theme.iconMoon
                    font.pixelSize: theme.fontSubhead
                    color: themeToggleBtn.hovered ? theme.accentWarm : theme.textSecondary
                    anchors.centerIn: parent

                    Behavior on color { ColorAnimation { duration: theme.animFast } }
                }

                MouseArea {
                    id: themeToggleBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.info("UI: Theme toggled to", theme.darkMode ? "light" : "dark")
                        theme.darkMode = !theme.darkMode
                    }

                    ToolTip.text: theme.darkMode ? "Switch to light mode" : "Switch to dark mode"
                    ToolTip.visible: themeToggleBtn.containsMouse
                    ToolTip.delay: 600
                }
            }

            // Settings button
            Rectangle {
                width: 32
                height: 32
                radius: theme.radiusMedium
                color: settingsBtn.hovered ? theme.bgOverlay : "transparent"

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: theme.iconSettings
                    font.pixelSize: theme.fontSubhead
                    color: settingsBtn.hovered ? theme.textPrimary : theme.textSecondary
                    anchors.centerIn: parent

                    Behavior on color { ColorAnimation { duration: theme.animFast } }
                }

                MouseArea {
                    id: settingsBtn
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.info("UI: Settings drawer opened")
                        settingsDrawer.open()
                    }
                }
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: 1
            implicitHeight: parent.height
            color: theme.borderSubtle

            Rectangle {
                anchors.centerIn: parent
                width: 3
                height: 32
                radius: 2
                color: parent.SplitHandle.hovered ? theme.accent : theme.bgOverlay
                opacity: parent.SplitHandle.hovered ? 1.0 : 0.6

                Behavior on color { ColorAnimation { duration: theme.animFast } }
                Behavior on opacity { NumberAnimation { duration: theme.animFast } }
            }
        }

        // Left sidebar
        Rectangle {
            SplitView.preferredWidth: 280
            SplitView.minimumWidth: 200
            SplitView.maximumWidth: 500
            color: theme.bgSurface

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Sidebar header
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: theme.sp40
                    color: theme.bgSurface2

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: theme.sp12
                        anchors.rightMargin: theme.sp8
                        spacing: theme.sp8

                        Label {
                            text: "Vault"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }

                        Item { Layout.fillWidth: true }

                        // Refresh button
                        Rectangle {
                            width: 28
                            height: 28
                            radius: theme.radiusSmall
                            color: refreshBtn.hovered ? theme.bgOverlay : "transparent"

                            Behavior on color { ColorAnimation { duration: theme.animFast } }

                            Label {
                                text: theme.iconRefresh
                                font.pixelSize: theme.fontBody
                                color: refreshBtn.hovered ? theme.textPrimary : theme.textSecondary
                                anchors.centerIn: parent

                                Behavior on color { ColorAnimation { duration: theme.animFast } }
                            }

                            MouseArea {
                                id: refreshBtn
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    console.info("UI: Vault refresh requested")
                                    fileTreeModel.refresh()
                                }

                                ToolTip.text: "Refresh"
                                ToolTip.visible: refreshBtn.containsMouse
                                ToolTip.delay: 600
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: theme.borderSubtle
                }

                // File browser
                FileBrowser {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    onFileSelected: function(path) {
                        root.importPanelVisible = false
                        root.skillPanelVisible = false
                        articleModel.loadFromFile(path)
                    }
                }
            }
        }

        // Right content area
        Rectangle {
            SplitView.fillWidth: true
            color: theme.bgBase

            // Article view
            ArticleView {
                anchors.fill: parent
                visible: !importPanelVisible && !skillPanelVisible
            }

            // Import panel
            ImportPanel {
                anchors.fill: parent
                visible: importPanelVisible
            }

            // Skill panel (Wiki Tools)
            SkillPanel {
                anchors.fill: parent
                visible: skillPanelVisible
            }
        }
    }

    // Track which view is active
    property bool importPanelVisible: false
    property bool skillPanelVisible: false

    // Empty state overlay
    Rectangle {
        anchors.centerIn: parent
        width: 420
        height: 340
        color: "transparent"
        visible: !articleModel.loaded && fileTreeModel.fileCount === 0
        opacity: visible ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: theme.animSlow } }

        Column {
            anchors.centerIn: parent
            spacing: theme.sp16

            // Animated hex icon
            Label {
                text: theme.iconHex
                font.pixelSize: theme.fontHero
                color: theme.accent
                anchors.horizontalCenter: parent.horizontalCenter
                opacity: 0.8

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 1.0; duration: 2000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.5; duration: 2000; easing.type: Easing.InOutSine }
                }
            }

            Label {
                text: "Welcome to Arkive"
                font.pixelSize: theme.fontTitle
                font.weight: Font.Bold
                color: theme.textPrimary
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Label {
                text: "Your vault is empty. Import data to get started."
                font.pixelSize: theme.fontBody
                color: theme.textSecondary
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Item { height: theme.sp8; width: 1 }

            // Steps
            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: theme.sp8

                Repeater {
                    model: [
                        { num: "1", label: "Set your vault path in Settings" },
                        { num: "2", label: "Import photos, messages, or notes" },
                        { num: "3", label: "Browse your personal wiki" }
                    ]

                    Row {
                        spacing: theme.sp12

                        Rectangle {
                            width: 24; height: 24
                            radius: theme.radiusPill
                            color: theme.bgOverlay

                            Label {
                                text: modelData.num
                                font.pixelSize: theme.fontSmall
                                font.weight: Font.DemiBold
                                color: theme.accent
                                anchors.centerIn: parent
                            }
                        }

                        Label {
                            text: modelData.label
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }

            Item { height: theme.sp8; width: 1 }

            // CTA button
            Rectangle {
                width: 160
                height: 40
                radius: theme.radiusMedium
                color: importBtnArea.pressed ? Qt.darker(theme.accent, 1.15)
                     : importBtnArea.containsMouse ? theme.accentHover
                     : theme.accent
                anchors.horizontalCenter: parent.horizontalCenter

                Behavior on color { ColorAnimation { duration: theme.animFast } }

                Label {
                    text: "Import Data..."
                    font.pixelSize: theme.fontBody
                    font.weight: Font.DemiBold
                    color: theme.bgBase
                    anchors.centerIn: parent
                }

                MouseArea {
                    id: importBtnArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        console.info("UI: Import panel opened from empty state")
                        root.importPanelVisible = true
                    }
                }
            }
        }
    }

    // Settings drawer
    Drawer {
        id: settingsDrawer
        width: 360
        height: root.height
        edge: Qt.RightEdge
        modal: true
        dim: true

        background: Rectangle {
            color: theme.bgSurface
            Rectangle {
                width: 1
                height: parent.height
                color: theme.borderSubtle
                anchors.left: parent.left
            }
        }

        SettingsPanel {
            anchors.fill: parent
            onCloseRequested: settingsDrawer.close()
        }
    }
}
