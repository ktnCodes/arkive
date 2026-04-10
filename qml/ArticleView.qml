import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: articleView

    // No article loaded state
    Label {
        anchors.centerIn: parent
        text: "Select an article from the sidebar"
        font.pixelSize: 16
        color: textSubtle
        visible: !articleModel.loaded
    }

    // Article content
    ScrollView {
        anchors.fill: parent
        anchors.margins: 24
        visible: articleModel.loaded
        clip: true

        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            width: articleView.width - 48
            spacing: 16

            // Title
            Label {
                text: articleModel.title
                font.pixelSize: 28
                font.bold: true
                color: textColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            // Backlinks bar
            Flow {
                Layout.fillWidth: true
                spacing: 8
                visible: articleModel.backlinks.length > 0

                Label {
                    text: "Links:"
                    font.pixelSize: 12
                    color: textSubtle
                }

                Repeater {
                    model: articleModel.backlinks

                    Rectangle {
                        width: backlinkLabel.width + 16
                        height: 24
                        radius: 12
                        color: bgOverlay

                        Label {
                            id: backlinkLabel
                            text: modelData
                            font.pixelSize: 12
                            color: accentColor
                            anchors.centerIn: parent
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                // Navigate to linked article
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
                color: borderColor
            }

            // Rendered HTML content
            Text {
                text: articleModel.htmlContent
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                color: textColor
                font.pixelSize: 15
                lineHeight: 1.6
                Layout.fillWidth: true

                onLinkActivated: function(link) {
                    // Handle backlink clicks
                    console.log("Link clicked:", link)
                }
            }

            // Section headings (table of contents)
            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 16
                height: tocColumn.height + 24
                color: bgSurface
                radius: 8
                visible: articleModel.sectionHeadings.length > 1

                ColumnLayout {
                    id: tocColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 12
                    spacing: 4

                    Label {
                        text: "Sections"
                        font.pixelSize: 12
                        font.bold: true
                        color: textSubtle
                    }

                    Repeater {
                        model: articleModel.sectionHeadings

                        Label {
                            text: "• " + modelData
                            font.pixelSize: 13
                            color: accentColor

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                            }
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.preferredHeight: 24 }
        }
    }
}
