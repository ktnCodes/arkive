import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: importPanel

    signal importStarted()
    signal importDone()

    function urlToLocalPath(urlValue) {
        var path = urlValue.toString()
        if (path.startsWith("file:///")) {
            path = path.substring(8)
        }
        return decodeURIComponent(path)
    }

    FolderDialog {
        id: folderDialog
        title: "Select a folder of photos"
        onAccepted: {
            photoIngestor.importFromFolder(importPanel.urlToLocalPath(selectedFolder))
        }
    }

    FileDialog {
        id: messageDbDialog
        title: "Select iMessage chat.db"
        fileMode: FileDialog.OpenFile
        nameFilters: ["SQLite database (*.db *.sqlite)", "All files (*)"]
        onAccepted: messageIngestor.importChatDatabase(importPanel.urlToLocalPath(selectedFile))
    }

    FolderDialog {
        id: snapchatFolderDialog
        title: "Select the main Snapchat export folder or its parent folder"
        onAccepted: snapchatIngestor.importExportFolder(importPanel.urlToLocalPath(selectedFolder))
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: theme.sp24
        spacing: theme.sp16

        // Header
        Label {
            text: "Import Data"
            font.pixelSize: theme.fontTitle
            font.weight: Font.Bold
            color: theme.textPrimary
        }

        Label {
            text: "Import personal data into your vault. Each source creates structured wiki entries."
            font.pixelSize: theme.fontSmall
            color: theme.textSecondary
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Item { height: theme.sp8 }

        // ─── Photos Source Card ───
        Rectangle {
            Layout.fillWidth: true
            height: photoCardContent.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: photoCardContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                RowLayout {
                    spacing: theme.sp12

                    Label {
                        text: theme.iconPhoto
                        font.pixelSize: theme.fontTitle
                        color: theme.accent
                    }

                    ColumnLayout {
                        spacing: theme.sp2
                        Layout.fillWidth: true

                        Label {
                            text: "Photos"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }
                        Label {
                            text: "Import JPEG photos with EXIF metadata (date, GPS, camera)"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                // Progress bar (visible during import)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: theme.sp4
                    visible: photoIngestor.running

                    Rectangle {
                        Layout.fillWidth: true
                        height: 6
                        radius: 3
                        color: theme.bgOverlay

                        Rectangle {
                            width: photoIngestor.totalFiles > 0
                                ? parent.width * (photoIngestor.progress / photoIngestor.totalFiles)
                                : 0
                            height: parent.height
                            radius: 3
                            color: theme.accent

                            Behavior on width { NumberAnimation { duration: theme.animFast } }
                        }
                    }

                    Label {
                        text: photoIngestor.statusText
                        font.pixelSize: theme.fontCaption
                        color: theme.textMuted
                    }
                }

                // Status text (visible after import)
                Label {
                    text: photoIngestor.statusText
                    font.pixelSize: theme.fontSmall
                    color: theme.accentGreen
                    visible: !photoIngestor.running && photoIngestor.statusText.length > 0
                            && photoIngestor.statusText.startsWith("Done")
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                // Actions
                RowLayout {
                    spacing: theme.sp8

                    Rectangle {
                        width: importBtnLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: importBtnArea.pressed ? Qt.darker(theme.accent, 1.15)
                             : importBtnArea.containsMouse ? theme.accentHover
                             : theme.accent
                        visible: !photoIngestor.running

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        Label {
                            id: importBtnLabel
                            text: "Select Folder..."
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: importBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: folderDialog.open()
                        }
                    }

                    // Cancel button
                    Rectangle {
                        width: cancelLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: cancelArea.containsMouse ? theme.bgOverlay : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1
                        visible: photoIngestor.running

                        Label {
                            id: cancelLabel
                            text: "Cancel"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: cancelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: photoIngestor.cancel()
                        }
                    }
                }
            }
        }

        // ─── iMessage Source Card ───
        Rectangle {
            Layout.fillWidth: true
            height: messageCardContent.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: messageCardContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                RowLayout {
                    spacing: theme.sp12

                    Label {
                        text: theme.iconChat
                        font.pixelSize: theme.fontTitle
                        color: theme.accent
                    }

                    ColumnLayout {
                        spacing: theme.sp2
                        Layout.fillWidth: true

                        Label {
                            text: "iMessages"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }
                        Label {
                            text: "Import conversations from an iTunes or Apple Devices backup chat.db"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: theme.sp4
                    visible: messageIngestor.running

                    Rectangle {
                        Layout.fillWidth: true
                        height: 6
                        radius: 3
                        color: theme.bgOverlay

                        Rectangle {
                            width: messageIngestor.totalChats > 0
                                ? parent.width * (messageIngestor.progress / messageIngestor.totalChats)
                                : 0
                            height: parent.height
                            radius: 3
                            color: theme.accent

                            Behavior on width { NumberAnimation { duration: theme.animFast } }
                        }
                    }

                    Label {
                        text: messageIngestor.statusText
                        font.pixelSize: theme.fontCaption
                        color: theme.textMuted
                    }
                }

                Label {
                    text: messageIngestor.statusText
                    font.pixelSize: theme.fontSmall
                    color: messageIngestor.statusText.startsWith("Done") ? theme.accentGreen : theme.textSecondary
                    visible: !messageIngestor.running && messageIngestor.statusText.length > 0
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: theme.sp8

                    Rectangle {
                        width: messageImportLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: messageImportArea.pressed ? Qt.darker(theme.accent, 1.15)
                             : messageImportArea.containsMouse ? theme.accentHover
                             : theme.accent
                        visible: !messageIngestor.running

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        Label {
                            id: messageImportLabel
                            text: "Select chat.db..."
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: messageImportArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: messageDbDialog.open()
                        }
                    }

                    Rectangle {
                        width: messageCancelLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: messageCancelArea.containsMouse ? theme.bgOverlay : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1
                        visible: messageIngestor.running

                        Label {
                            id: messageCancelLabel
                            text: "Cancel"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: messageCancelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: messageIngestor.cancel()
                        }
                    }
                }
            }
        }

        // ─── Snapchat Source Card ───
        Rectangle {
            Layout.fillWidth: true
            height: snapchatCardContent.height + theme.sp32
            radius: theme.radiusMedium
            color: theme.bgSurface
            border.color: theme.borderSubtle
            border.width: 1

            ColumnLayout {
                id: snapchatCardContent
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: theme.sp16
                spacing: theme.sp12

                RowLayout {
                    spacing: theme.sp12

                    Label {
                        text: theme.iconSnap
                        font.pixelSize: theme.fontTitle
                        color: theme.accent
                    }

                    ColumnLayout {
                        spacing: theme.sp2
                        Layout.fillWidth: true

                        Label {
                            text: "Snapchat"
                            font.pixelSize: theme.fontBody
                            font.weight: Font.DemiBold
                            color: theme.textPrimary
                        }
                        Label {
                            text: "Import a standard Snapchat export. You can select the main export folder or the parent folder that contains the split media parts."
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: theme.sp4
                    visible: snapchatIngestor.running

                    Rectangle {
                        Layout.fillWidth: true
                        height: 6
                        radius: 3
                        color: theme.bgOverlay

                        Rectangle {
                            width: snapchatIngestor.totalFiles > 0
                                ? parent.width * (snapchatIngestor.progress / snapchatIngestor.totalFiles)
                                : 0
                            height: parent.height
                            radius: 3
                            color: theme.accent

                            Behavior on width { NumberAnimation { duration: theme.animFast } }
                        }
                    }

                    Label {
                        text: snapchatIngestor.statusText
                        font.pixelSize: theme.fontCaption
                        color: theme.textMuted
                    }
                }

                Label {
                    text: snapchatIngestor.statusText
                    font.pixelSize: theme.fontSmall
                    color: snapchatIngestor.statusText.startsWith("Done") ? theme.accentGreen : theme.textSecondary
                    visible: !snapchatIngestor.running && snapchatIngestor.statusText.length > 0
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: theme.sp8

                    Rectangle {
                        width: snapchatImportLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: snapchatImportArea.pressed ? Qt.darker(theme.accent, 1.15)
                             : snapchatImportArea.containsMouse ? theme.accentHover
                             : theme.accent
                        visible: !snapchatIngestor.running

                        Behavior on color { ColorAnimation { duration: theme.animFast } }

                        Label {
                            id: snapchatImportLabel
                            text: "Select Export Folder..."
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.bgBase
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: snapchatImportArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: snapchatFolderDialog.open()
                        }
                    }

                    Rectangle {
                        width: snapchatCancelLabel.width + theme.sp24
                        height: 36
                        radius: theme.radiusMedium
                        color: snapchatCancelArea.containsMouse ? theme.bgOverlay : theme.bgSurface2
                        border.color: theme.borderSubtle
                        border.width: 1
                        visible: snapchatIngestor.running

                        Label {
                            id: snapchatCancelLabel
                            text: "Cancel"
                            font.pixelSize: theme.fontSmall
                            color: theme.textSecondary
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            id: snapchatCancelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: snapchatIngestor.cancel()
                        }
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
