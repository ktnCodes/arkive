import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: articleView

    // No article loaded state
    Column {
        anchors.centerIn: parent
        spacing: theme.sp12
        visible: !articleModel.loaded
        opacity: visible ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: theme.animNormal } }

        Label {
            text: theme.iconMarkdown
            font.pixelSize: theme.fontHero
            color: theme.textMuted
            anchors.horizontalCenter: parent.horizontalCenter
            opacity: 0.3
        }

        Label {
            text: "Select an article from the sidebar"
            font.pixelSize: theme.fontSubhead
            color: theme.textMuted
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    // Article content
    Flickable {
        id: articleFlickable
        anchors.fill: parent
        visible: articleModel.loaded
        clip: true
        contentWidth: width
        contentHeight: contentCol.childrenRect.height + contentCol.y + 120
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

        Item {
            width: articleFlickable.width
            height: contentCol.childrenRect.height + contentCol.y + 120

            ColumnLayout {
                id: contentCol
                width: Math.min(parent.width - theme.sp48 * 2, 720)
                x: (parent.width - width) / 2
                y: theme.sp32
                spacing: theme.sp16

                // Title
                Label {
                    text: articleModel.title
                    font.pixelSize: theme.fontDisplay
                    font.weight: Font.Bold
                    color: theme.textPrimary
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    lineHeight: 1.2
                }

                // Backlinks bar
                Flow {
                    Layout.fillWidth: true
                    spacing: theme.sp8
                    visible: articleModel.backlinks.length > 0

                    Label {
                        text: theme.iconLink + " Links"
                        font.pixelSize: theme.fontSmall
                        font.weight: Font.DemiBold
                        color: theme.textMuted
                        height: 28
                        verticalAlignment: Text.AlignVCenter
                    }

                    Repeater {
                        model: articleModel.backlinks

                        Rectangle {
                            width: backlinkLabel.width + theme.sp16
                            height: 28
                            radius: theme.radiusPill
                            color: backlinkArea.containsMouse ? theme.bgOverlay : theme.bgSurface
                            border.color: backlinkArea.containsMouse ? theme.accent : theme.borderSubtle
                            border.width: 1

                            Behavior on color { ColorAnimation { duration: theme.animFast } }
                            Behavior on border.color { ColorAnimation { duration: theme.animFast } }

                            Label {
                                id: backlinkLabel
                                text: modelData
                                font.pixelSize: theme.fontSmall
                                color: theme.accent
                                anchors.centerIn: parent
                            }

                            MouseArea {
                                id: backlinkArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    console.log("Navigate to:", modelData)
                                }
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

                // Rendered HTML content
                Text {
                    text: articleModel.htmlContent
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    color: theme.textPrimary
                    font.pixelSize: 15
                    lineHeight: 1.7
                    Layout.fillWidth: true

                    onLinkActivated: function(link) {
                        console.log("Link clicked:", link)
                    }
                }

                // Table of contents
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: theme.sp16
                    implicitHeight: tocColumn.implicitHeight + theme.sp24
                    color: theme.bgSurface
                    radius: theme.radiusMedium
                    border.color: theme.borderSubtle
                    border.width: 1
                    visible: articleModel.sectionHeadings.length > 1

                    ColumnLayout {
                        id: tocColumn
                        width: parent.width - theme.sp24
                        x: theme.sp12
                        y: theme.sp12
                        spacing: theme.sp4

                        Label {
                            text: "On this page"
                            font.pixelSize: theme.fontSmall
                            font.weight: Font.DemiBold
                            color: theme.textMuted
                            Layout.bottomMargin: theme.sp4
                        }

                        Repeater {
                            model: articleModel.sectionHeadings

                            Item {
                                Layout.fillWidth: true
                                height: 28

                                Rectangle {
                                    anchors.fill: parent
                                    radius: theme.radiusSmall
                                    color: tocItemArea.containsMouse ? theme.bgSurface2 : "transparent"

                                    Behavior on color { ColorAnimation { duration: theme.animFast } }

                                    Label {
                                        text: modelData
                                        font.pixelSize: theme.fontSmall
                                        color: tocItemArea.containsMouse ? theme.accent : theme.textSecondary
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: theme.sp8

                                        Behavior on color { ColorAnimation { duration: theme.animFast } }
                                    }
                                }

                                MouseArea {
                                    id: tocItemArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                }
                            }
                        }
                    }
                }

                // Bottom spacer
                Item { Layout.preferredHeight: theme.sp48 }
            }
        }
    }
}
