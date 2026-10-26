
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root

    required property var controller

    property url selectedFolder: ""
    property int processed: 0
    property int total: 0

    property string statusText:
        "Select a folder to begin."

    property var searchResults: []
    property string searchSummary:
        "Index a folder, then search its contents."

    function performSearch() {
        if (searchField.text.trim().length === 0 ||
            controller.busy ||
            controller.searching) {
            return
        }

        root.searchResults = []
        root.searchSummary = "Searching..."

        controller.searchFiles(searchField.text)
    }

    width: 960
    height: 760

    minimumWidth: 680
    minimumHeight: 580

    visible: true
    title: "FastIndex"
    color: "#0f172a"

    FolderDialog {
        id: folderDialog
        title: "Select folder to index"

        onAccepted: {
            root.selectedFolder = folderDialog.selectedFolder
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

        function onSearchFinished(results, total) {
            root.searchResults = results

            root.searchSummary = total === 0
                ? "No matching files found."
                : total + " matching files"
                  + (total > results.length
                     ? " (showing first " + results.length + ")"
                     : "")
        }

        function onSearchFailed(message) {
            root.searchResults = []
            root.searchSummary = "Search error: " + message
        }

        function onFailed(message) {
            root.statusText = "Error: " + message
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 16

        // Header
        Text {
            text: "FastIndex"
            font.pixelSize: 34
            font.bold: true
            color: "#f8fafc"
        }

        Text {
            text: "Fast local file indexing with C++20"
            font.pixelSize: 14
            color: "#94a3b8"
        }

        // Folder Selection
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 105
            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Text {
                    text: "Selected folder"
                    color: "#cbd5e1"
                    font.pixelSize: 14
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "Browse..."
                        enabled: !root.controller.busy

                        onClicked: folderDialog.open()
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.selectedFolder.toString()
                              || "No folder selected"
                        color: "#f8fafc"
                        elide: Text.ElideMiddle
                        font.pixelSize: 13
                    }
                }
            }
        }

        // Indexing Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: "Start indexing"

                enabled: !root.controller.busy
                         && !root.controller.searching
                         && root.selectedFolder.toString() !== ""

                onClicked: {
                    root.processed = 0
                    root.total = 0
                    root.searchResults = []
                    root.searchSummary = "Indexing in progress..."
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

        // Indexing Progress
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 130
            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: "Indexing progress"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#f8fafc"
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: root.total > 0 ? root.total : 1
                    value: root.processed

                    indeterminate:
                        root.controller.busy && root.total === 0
                }

                Text {
                    text: root.processed + " / "
                          + root.total + " files processed"
                    color: "#cbd5e1"
                    font.pixelSize: 13
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.statusText
            color: "#5eead4"
            wrapMode: Text.WordWrap
            font.pixelSize: 13
        }

        // Search Panel
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Layout.minimumHeight: 180

            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: "Search indexed files"
                    font.pixelSize: 18
                    font.bold: true
                    color: "#f8fafc"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    TextField {
                        id: searchField

                        Layout.fillWidth: true

                        placeholderText: "Enter a search term..."

                        enabled: !root.controller.busy
                                 && !root.controller.searching

                        onAccepted: root.performSearch()
                    }

                    Button {
                        text: root.controller.searching
                              ? "Searching..." : "Search"

                        enabled: searchField.text.trim().length > 0
                                 && !root.controller.busy
                                 && !root.controller.searching

                        onClicked: root.performSearch()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.searchSummary
                    color: "#94a3b8"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }

                ListView {
                    id: resultList

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    clip: true
                    spacing: 8

                    model: root.searchResults

                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }

                    delegate: Rectangle {
                        id: resultCard

                        required property var modelData

                        width: resultList.width
                        height: 105

                        radius: 8
                        color: "#334155"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 5

                            Text {
                                Layout.fillWidth: true
                                text: resultCard.modelData.name
                                color: "#5eead4"
                                font.bold: true
                                font.pixelSize: 14
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: resultCard.modelData.path
                                color: "#94a3b8"
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }

                            Text {
                                Layout.fillWidth: true
                                text: resultCard.modelData.snippet
                                color: "#f8fafc"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                root.controller.openFile(
                                    resultCard.modelData.path
                                )
                            }
                        }
                    }
                }
            }
        }
    }
}
