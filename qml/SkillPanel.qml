import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: skillPanel

    // Auto-check Ollama on panel load
    Component.onCompleted: cloudLlmClient.checkOllamaConnection()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.sp24
        spacing: theme.sp16

        // Header
        Label {
            text: "Wiki Tools"
            font.pixelSize: theme.fontTitle
            font.weight: Font.Bold
            color: theme.textPrimary
        }

        Label {
            text: "Compile raw vault entries into structured wiki articles using a local or cloud LLM."
            font.pixelSize: theme.fontSmall
            color: theme.textSecondary
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Item { height: theme.sp8 }

        // ─── LLM Provider Card ───
        Rectangle {
            Layout.fillWidth: true
            height: llmStatusCol.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: llmStatusCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                // Status indicator
                RowLayout {
                    spacing: theme.sp8

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: cloudLlmClient.ready ? theme.accentGreen : theme.accentRed
                    }

                    Label {
                        text: cloudLlmClient.ready
                            ? "LLM Ready (" + cloudLlmClient.provider + ")"
                            : "LLM Not Connected"
                        font.pixelSize: theme.fontSmall
                        font.weight: Font.DemiBold
                        color: theme.textPrimary
                    }
                }

                // Provider toggle row
                RowLayout {
                    spacing: theme.sp8
                    Layout.fillWidth: true

                    Repeater {
                        model: ["Ollama", "Claude", "OpenAI"]

                        Rectangle {
                            required property string modelData
                            required property int index
                            width: providerLabel.width + theme.sp16
                            height: 28
                            radius: theme.radiusSmall
                            color: cloudLlmClient.provider === modelData
                                ? theme.accent : "transparent"
                            border.color: theme.borderSubtle
                            border.width: cloudLlmClient.provider === modelData ? 0 : 1

                            Label {
                                id: providerLabel
                                text: modelData
                                font.pixelSize: theme.fontSmall
                                font.weight: Font.DemiBold
                                color: cloudLlmClient.provider === modelData
                                    ? theme.bgBase : theme.textSecondary
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    console.info("UI: Switch LLM provider to " + modelData)
                                    if (modelData === "Ollama") {
                                        cloudLlmClient.setBackendProvider(0)
                                        cloudLlmClient.checkOllamaConnection()
                                    } else if (modelData === "Claude") {
                                        cloudLlmClient.setBackendProvider(1)
                                    } else {
                                        cloudLlmClient.setBackendProvider(2)
                                    }
                                }
                            }
                        }
                    }
                }

                // Ollama section — shown when Ollama is selected
                ColumnLayout {
                    spacing: theme.sp8
                    Layout.fillWidth: true
                    visible: cloudLlmClient.provider === "Ollama"

                    Label {
                        text: cloudLlmClient.ollamaConnected
                            ? "Connected to Ollama at localhost:11434"
                            : "Ollama not detected. Install from ollama.com then run: ollama pull llama3.2"
                        font.pixelSize: theme.fontSmall
                        color: cloudLlmClient.ollamaConnected ? theme.textSecondary : theme.accentRed
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    // Retry connection button
                    Rectangle {
                        visible: !cloudLlmClient.ollamaConnected
                        width: retryLabel.width + theme.sp16
                        height: 28
                        radius: theme.radiusSmall
                        color: retryArea.containsMouse ? theme.accentHover : theme.accent

                        Label {
                            id: retryLabel
                            text: "Retry Connection"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: retryArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Retry Ollama connection")
                                cloudLlmClient.checkOllamaConnection()
                            }
                        }
                    }
                }

                // API Key section — shown for Claude or OpenAI
                RowLayout {
                    spacing: theme.sp8
                    Layout.fillWidth: true
                    visible: cloudLlmClient.provider !== "Ollama" && !cloudLlmClient.ready

                    Label {
                        text: "API Key"
                        font.pixelSize: theme.fontSmall
                        color: theme.textSecondary
                        Layout.preferredWidth: 56
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 32
                        radius: theme.radiusSmall
                        color: theme.bgOverlay
                        border.color: apiKeyField.activeFocus ? theme.accent : theme.borderSubtle
                        border.width: 1

                        TextInput {
                            id: apiKeyField
                            anchors.fill: parent
                            anchors.margins: theme.sp8
                            font.pixelSize: theme.fontSmall
                            color: theme.textPrimary
                            clip: true
                            echoMode: TextInput.Password

                            Text {
                                anchors.fill: parent
                                text: cloudLlmClient.provider === "Claude" ? "sk-ant-..." : "sk-..."
                                font: parent.font
                                color: theme.textMuted
                                visible: !parent.text && !parent.activeFocus
                            }

                            onAccepted: {
                                if (text.length > 0) {
                                    console.info("UI: API key set")
                                    cloudLlmClient.apiKey = text
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: setKeyLabel.width + theme.sp16
                        height: 32
                        radius: theme.radiusSmall
                        color: setKeyArea.containsMouse ? theme.accentHover : theme.accent

                        Label {
                            id: setKeyLabel
                            text: "Set"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: setKeyArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (apiKeyField.text.length > 0) {
                                    console.info("UI: API key set via button")
                                    cloudLlmClient.apiKey = apiKeyField.text
                                }
                            }
                        }
                    }
                }
            }
        }

        // ─── Compile Card ───
        Rectangle {
            Layout.fillWidth: true
            height: compileCol.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: compileCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                RowLayout {
                    spacing: theme.sp12

                    Label {
                        text: "⬡"
                        font.pixelSize: theme.fontTitle
                        color: theme.accent
                    }

                    ColumnLayout {
                        spacing: theme.sp2
                        Layout.fillWidth: true

                        Label {
                            text: "Compile Wiki Articles"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }
                        Label {
                            text: "Scan raw entries and compile person summaries, monthly events, and topic clusters."
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                // Progress
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: theme.sp4
                    visible: wikiCompiler.running

                    Rectangle {
                        Layout.fillWidth: true
                        height: 6
                        radius: 3
                        color: theme.bgOverlay

                        Rectangle {
                            width: wikiCompiler.totalCandidates > 0
                                ? parent.width * (wikiCompiler.progress / wikiCompiler.totalCandidates)
                                : 0
                            height: parent.height
                            radius: 3
                            color: theme.accent

                            Behavior on width { NumberAnimation { duration: theme.animFast } }
                        }
                    }

                    Label {
                        text: wikiCompiler.statusText
                        font.pixelSize: theme.fontCaption
                        color: theme.textMuted
                    }
                }

                // Status text (when not running)
                Label {
                    text: wikiCompiler.statusText
                    font.pixelSize: theme.fontSmall
                    color: wikiCompiler.statusText.startsWith("Done") ? theme.accentGreen : theme.textSecondary
                    visible: !wikiCompiler.running && wikiCompiler.statusText.length > 0
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                // Actions
                RowLayout {
                    spacing: theme.sp8

                    // Scan button
                    Rectangle {
                        width: scanLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: scanArea.pressed ? Qt.darker(theme.bgOverlay, 1.15)
                             : scanArea.containsMouse ? theme.bgOverlay
                             : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1
                        visible: !wikiCompiler.running

                        Label {
                            id: scanLabel
                            text: "Scan Candidates"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: scanArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Wiki scan candidates clicked")
                                wikiCompiler.scanCandidates()
                            }
                        }
                    }

                    // Compile All button
                    Rectangle {
                        width: compileLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: compileArea.pressed ? Qt.darker(theme.accent, 1.15)
                             : compileArea.containsMouse ? theme.accentHover
                             : theme.accent
                        visible: !wikiCompiler.running && cloudLlmClient.ready

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        Label {
                            id: compileLabel
                            text: "Test 5"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: compileArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Wiki compile test batch of 5 clicked")
                                wikiCompiler.compileBatch(5)
                            }
                        }
                    }

                    // Compile All button
                    Rectangle {
                        width: compileAllLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: compileAllArea.pressed ? Qt.darker(theme.accent, 1.15)
                             : compileAllArea.containsMouse ? theme.accentHover
                             : theme.accent
                        visible: !wikiCompiler.running && cloudLlmClient.ready
                            && wikiCompiler.totalCandidates > 0

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        Label {
                            id: compileAllLabel
                            text: "Compile All (" + wikiCompiler.totalCandidates + ")"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: compileAllArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Wiki compile all clicked")
                                wikiCompiler.compileAll()
                            }
                        }
                    }

                    // Cancel button
                    Rectangle {
                        width: compileCancelLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: compileCancelArea.containsMouse ? theme.bgOverlay : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1
                        visible: wikiCompiler.running

                        Label {
                            id: compileCancelLabel
                            text: "Cancel"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: compileCancelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Wiki compile cancel clicked")
                                wikiCompiler.cancel()
                            }
                        }
                    }
                }
            }
        }

        // ─── Rebuild Index Card ───
        Rectangle {
            Layout.fillWidth: true
            height: indexCol.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: indexCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                RowLayout {
                    spacing: theme.sp12

                    Label {
                        text: theme.iconRefresh
                        font.pixelSize: theme.fontTitle
                        color: theme.accent
                    }

                    ColumnLayout {
                        spacing: theme.sp2
                        Layout.fillWidth: true

                        Label {
                            text: "Rebuild Indexes"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }
                        Label {
                            text: "Regenerate _index.md files for each vault section."
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                RowLayout {
                    spacing: theme.sp8

                    Rectangle {
                        width: rebuildLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: rebuildArea.pressed ? Qt.darker(theme.bgOverlay, 1.15)
                             : rebuildArea.containsMouse ? theme.bgOverlay
                             : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1

                        Label {
                            id: rebuildLabel
                            text: "Rebuild"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: rebuildArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.info("UI: Rebuild indexes clicked")
                                indexManager.rebuildIndexes()
                                fileTreeModel.refresh()
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
