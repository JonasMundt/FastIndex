
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

ApplicationWindow {
    id: root

    required property var controller

    property url selectedFolder: ""
    property int processed: 0
    property int total: 0
    property string statusText: "Select a folder to begin."

    width: 900
    height: 600
    minimumWidth: 650
    minimumHeight: 450

    visible: true
    title: "FastIndex"
    color: "#0f172a"

    FolderDialog {
        id: folderDialog
        title: "Select folder to index"

        onAccepted: {
            root.selectedFolder = selectedFolder
            root.statusText = "Folder selected. Ready to index."
        }
    }

    Connections {
        target: root.controller

        function onProgressUpdated(processed, total) {
            root.processed = processed
            root.total = total
        }

        function onIndexingFinished(cancelled, indexed, skipped) {
            root.statusText = cancelled
                ? "Indexing cancelled. Indexed: " + indexed
                : "Indexing completed! Indexed: " + indexed
                  + ", skipped: " + skipped
        }

        function onFailed(message) {
            root.statusText = "Error: " + message
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        Text {
            text: "FastIndex"
            font.pixelSize: 34
            font.bold: true
            color: "#f8fafc"
        }

        Text {
            text: "Fast local file indexing with C++20"
            font.pixelSize: 15
            color: "#94a3b8"
        }

        Rectangle {
            width: parent.width
            height: 110
            radius: 12
            color: "#1e293b"

            Column {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12

                Text {
                    text: "Selected folder"
                    color: "#cbd5e1"
                    font.pixelSize: 14
                }

                Row {
                    spacing: 14

                    Button {
                        text: "Browse..."
                        enabled: !root.controller.busy

                        onClicked: folderDialog.open()
                    }

                    Text {
                        width: 570
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.selectedFolder.toString() || "No folder selected"
                        color: "#f8fafc"
                        elide: Text.ElideMiddle
                        font.pixelSize: 13
                    }
                }
            }
        }

        Row {
            spacing: 14

            Button {
                text: "Start indexing"

                enabled: !root.controller.busy
                         && root.selectedFolder.toString() !== ""

                onClicked: {
                    root.processed = 0
                    root.total = 0
                    root.statusText = "Indexing started..."

                    root.controller.startIndexing(
                        root.selectedFolder
                    )
                }
            }

            Button {
                text: "Cancel"
                enabled: root.controller.busy

                onClicked: {
                    root.statusText = "Cancelling..."
                    root.controller.cancelIndexing()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 150
            radius: 12
            color: "#1e293b"

            Column {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                Text {
                    text: "Indexing progress"
                    font.pixelSize: 17
                    font.bold: true
                    color: "#f8fafc"
                }

                ProgressBar {
                    width: parent.width

                    from: 0
                    to: root.total > 0 ? root.total : 1
                    value: root.processed

                    indeterminate:
                        root.controller.busy && root.total === 0
                }

                Text {
                    text: root.processed + " / " + root.total + " files processed"
                    color: "#cbd5e1"
                    font.pixelSize: 13
                }
            }
        }

        Text {
            width: parent.width
            text: root.statusText
            color: "#5eead4"
            wrapMode: Text.WordWrap
            font.pixelSize: 14
        }
    }
}
