import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd

Page {
    id: compressionPage

    QmlCompressor {
        id: compressor
    }

    Item {
        width: parent.width   * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15
            Label {
                text: qsTr("Blocking compression")
                font.pixelSize: 20
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Compresses a folder into a single .zst archive. This runs synchronously on the UI thread, the window will not respond while it works, that is deliberate, this page exists to show the plain blocking API without any async machinery involved.")
                font.pixelSize: 13
                opacity: 0.75
            }
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Progress:") }
                ProgressBar {
                    Layout.fillWidth: true
                    value: compressor.current
                    from: 0
                    to: compressor.total > 0 ? compressor.total : 100
                }
                Label {
                    text: compressor.total > 0 ? Math.round((compressor.current / compressor.total) * 100) + "%" : "0%"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: compressorPrivate.hasFolder
                        ? qsTr("Target path: ") + compressor.input
                        : qsTr("No target folder selected.")
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    wrapMode: Text.WordWrap
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: compressorFolderPicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Compression level: %1").arg(compressor.compressionLevel)
                }
                Button {
                    text: qsTr("Options...")
                    onClicked: optionsPopup.open()
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: qsTr("Start compression")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 250
                enabled: compressorPrivate.hasFolder
                onClicked: {
                    if (compressor.compress()) {
                        compressorDialog.text = qsTr("Compression complete")
                        compressorDialog.informativeText = qsTr("Package compiled successfully.")
                    } else {
                        compressorDialog.text = qsTr("Compression failed")
                        compressorDialog.informativeText = compressor.errorString
                    }
                    compressorDialog.open()
                }
            }
        }
    }

    FolderDialog {
        id: compressorFolderPicker
        title: qsTr("Select input target directory")
        onAccepted: {
            let cleanPath = selectedFolder.toString()
            if (cleanPath.startsWith("file://"))
                cleanPath = cleanPath.replace("file://", "")

            compressor.input = cleanPath
            compressor.output = cleanPath + ".zst"
            compressorPrivate.hasFolder = true
        }
    }

    MessageDialog {
        id: compressorDialog
        buttons: MessageDialog.Ok
    }

    Item {
        id: compressorPrivate
        property bool hasFolder: false
    }

    ZStdOptionsPopup {
        id: optionsPopup
        target: compressor
        width: parent.width * 0.98
        height: parent.height * 0.98
        anchors.centerIn: parent
    }
}
