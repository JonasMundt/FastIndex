
import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root

    required property var controller

    Material.theme: Material.Dark
    Material.accent: Material.Teal
    Material.primary: Material.BlueGrey

    // Folder comes directly from saved C++ settings.
    readonly property url selectedFolder:
        root.controller.lastFolder

    property int processed: 0
    property int total: 0

    readonly property int progressPercent:
        total > 0
        ? Math.min(100, Math.round(processed * 100 / total))
        : 0

    property string statusText:
        "Select a folder to get started."

    property var searchResults: []
    property string searchSummary:
        "Index a folder and search its contents."

    function highlightSnippet(value) {
        let escaped = String(value)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")

        return escaped.replace(
            /\[([^\]]+)\]/g,
            '<font color="#5eead4"><b>$1</b></font>'
        )
    }

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

    width: 1050
    height: 760

    minimumWidth: 720
    minimumHeight: 600

    visible: true
    title: "FastIndex"
    color: "#0f172a"

    // ------------------------------------------
    // Folder Selection
    // ------------------------------------------

    FolderDialog {
        id: folderDialog
        title: "Select folder to index"

        onAccepted: {
            root.controller.setLastFolder(
                folderDialog.selectedFolder
            )

            root.statusText =
                "Folder selected. Ready to index."
        }
    }

    // ------------------------------------------
    // Controller Signals
    // ------------------------------------------

    Connections {
        target: root.controller

        function onProgressUpdated(processed, total) {
            root.processed = processed
            root.total = total
        }

        function onIndexingFinished(cancelled, indexed, skipped) {
            root.statusText = cancelled
                ? "Indexing cancelled. Indexed: " + indexed
                : "Indexing complete! " + indexed
                  + " files indexed, " + skipped + " skipped."
        }

        function onFailed(message) {
            root.statusText = "Error: " + message
        }

        function onSearchFinished(results, total) {
            root.searchResults = results

            root.searchSummary = total === 0
                ? "No matching files found."
                : total + " matching files"
                  + (total > results.length
                    ? " - showing first " + results.length
                    : "")
        }

        function onSearchFailed(message) {
            root.searchResults = []
            root.searchSummary = "Search error: " + message
        }
    }

    // ------------------------------------------
    // Main Layout
    // ------------------------------------------

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 14

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: "FastIndex"
                    font.pixelSize: 32
                    font.bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "FAST LOCAL FILE SEARCH"
                    font.pixelSize: 11
                    font.letterSpacing: 1.5
                    color: "#94a3b8"
                }
            }

            Rectangle {
                implicitWidth: 135
                implicitHeight: 34
                radius: 17
                color: "#193c3b"

                Row {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#5eead4"
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Text {
                        text: root.controller.busy
                            ? "INDEXING"
                            : root.controller.searching
                              ? "SEARCHING"
                              : "LOCAL INDEX"

                        color: "#5eead4"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }
            }
        }

        // --------------------------------------
        // Folder Card
        // --------------------------------------

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 118
            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                Text {
                    text: "SELECTED DIRECTORY"
                    font.pixelSize: 11
                    font.bold: true
                    font.letterSpacing: 1
                    color: "#94a3b8"
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Button {
                        text: "Browse..."

                        enabled: !root.controller.busy
                                 && !root.controller.searching

                        onClicked: folderDialog.open()
                    }

                    Text {
                        Layout.fillWidth: true

                        text: root.selectedFolder.toString()
                            || "No folder selected"

                        color: "#f8fafc"
                        font.pixelSize: 13
                        elide: Text.ElideMiddle
                    }
                }
            }
        }

        // --------------------------------------
        // Indexing Actions and Worker Setting
        // --------------------------------------

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                text: "Start indexing"
                highlighted: true

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

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: "Worker threads"
                font.pixelSize: 12
                color: "#94a3b8"
            }

            SpinBox {
                id: workerSelector

                from: 1
                to: 16
                stepSize: 1

                value: root.controller.workerCount

                editable: false

                enabled: !root.controller.busy
                         && !root.controller.searching

                Layout.preferredWidth: 112

                onValueModified: {
                    root.controller.setWorkerCount(value)
                }
            }
        }

        // --------------------------------------
        // Progress Card
        // --------------------------------------

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 126

            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Indexing progress"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#f8fafc"
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        text: root.progressPercent + "%"
                        font.pixelSize: 15
                        font.bold: true
                        color: "#5eead4"
                    }
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

                    color: "#94a3b8"
                    font.pixelSize: 12
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.statusText
            color: "#5eead4"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }

        // --------------------------------------
        // Search Card
        // --------------------------------------

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 170

            radius: 12
            color: "#1e293b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "Search your files"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#f8fafc"
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Text {
                        text: "FULL-TEXT SEARCH"
                        font.pixelSize: 10
                        font.letterSpacing: 1
                        color: "#64748b"
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    TextField {
                        id: searchField

                        Layout.fillWidth: true

                        placeholderText:
                            "Search indexed content..."

                        selectByMouse: true

                        enabled: !root.controller.busy
                                 && !root.controller.searching

                        onAccepted: root.performSearch()
                    }

                    Button {
                        text: root.controller.searching
                            ? "Searching..."
                            : "Search"

                        highlighted: true

                        enabled:
                            searchField.text.trim().length > 0
                            && !root.controller.busy
                            && !root.controller.searching

                        onClicked: root.performSearch()
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.searchSummary
                    color: "#94a3b8"
                    font.pixelSize: 12
                }

                // ----------------------------------
                // Search Results
                // ----------------------------------

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
                        height: 104

                        radius: 9

                        color: cardMouse.containsMouse
                            ? "#334155"
                            : "#273449"

                        border.width: cardMouse.containsMouse
                            ? 1 : 0

                        border.color: "#5eead4"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 5

                            RowLayout {
                                Layout.fillWidth: true

                                Text {
                                    Layout.fillWidth: true

                                    text: resultCard.modelData.name

                                    color: "#5eead4"
                                    font.pixelSize: 14
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    text: "OPEN"
                                    font.pixelSize: 10
                                    color: "#94a3b8"
                                }
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

                                text: root.highlightSnippet(
                                    resultCard.modelData.snippet
                                )

                                textFormat: Text.RichText
                                color: "#f8fafc"
                                font.pixelSize: 12

                                maximumLineCount: 2
                                elide: Text.ElideRight
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                            }
                        }

                        MouseArea {
                            id: cardMouse

                            anchors.fill: parent
                            hoverEnabled: true

                            cursorShape: Qt.PointingHandCursor

                            onClicked: {
                                root.controller.openFile(
                                    resultCard.modelData.path
                                )
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignHCenter

                    visible: root.searchResults.length === 0
                             && !root.controller.searching

                    text: "Your search results will appear here."
                    color: "#64748b"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
