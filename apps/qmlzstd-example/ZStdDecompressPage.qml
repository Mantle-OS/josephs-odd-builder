import QtQuick
import QtCore
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd

Page {
    id: decompressionPage

    QmlDecompressor { id: decompressor }


    Item {
        width: parent.width   * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15
            Label {
                text: qsTr("Blocking decompression")
                font.pixelSize: 20
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Extracts a .zst archive back into a folder. Same blocking behavior as the compression tab, the call returns only once extraction has fully finished.")
                font.pixelSize: 13
                opacity: 0.75
            }
            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Progression:") }
                ProgressBar {
                    Layout.fillWidth: true
                    value: decompressor.current
                    from: 0
                    to: decompressor.total > 0 ? decompressor.total : 100
                }
                Label {
                    text: decompressor.total > 0 ?
                              Math.round((decompressor.current / decompressor.total) * 100) + "%" :
                              "0%"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: decompressorPrivate.hasFile ?
                              qsTr("Target archive: ") + decompressor.input :
                              qsTr("No compressed archive selected.")
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    wrapMode: Text.WordWrap
                }
                Button {
                    text: qsTr("Browse archives...")
                    onClicked: decompressorFilePicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Preserve symlinks: %1").arg(decompressor.preserveSymlinks ? qsTr("yes") : qsTr("no"))
                }
                Button {
                    text: qsTr("Options...")
                    onClicked: optionsPopup.open()
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: qsTr("Start decompression")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 250
                enabled: decompressorPrivate.hasFile
                onClicked: {
                    if (decompressor.decompress()) {
                        decompressorDialog.text = qsTr("Decompression complete")
                        decompressorDialog.informativeText = qsTr("Archive decompressed successfully.")
                    } else {
                        decompressorDialog.text = qsTr("Decompression failed")
                        decompressorDialog.informativeText = decompressor.errorString
                    }
                    decompressorDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: decompressorFilePicker
        title: qsTr("Select Zstd archive for extraction")
        currentFolder: StandardPaths.standardLocations(StandardPaths.DownloadLocation)[0]
        nameFilters: [ "Zstd compressed archives (*.zst)", "All files (*)" ]
        onAccepted: {
            let cleanPath = selectedFile.toString()
            if (cleanPath.startsWith("file://"))
                cleanPath = cleanPath.replace("file://", "")

            decompressor.input = cleanPath
            if (cleanPath.endsWith(".zst")) {
                decompressor.output = cleanPath.substring(0, cleanPath.length - 4)
            } else {
                decompressor.output = cleanPath + ".extracted"
            }
            decompressorPrivate.hasFile = true
        }
    }

    MessageDialog {
        id: decompressorDialog
        buttons: MessageDialog.Ok
    }

    Item {
        id: decompressorPrivate
        property bool hasFile: false
    }

    ZStdOptionsPopup {
        id: optionsPopup
        target: decompressor
        width: parent.width * 0.98
        height: parent.height * 0.98
        anchors.centerIn: parent
    }
}
