import QtQuick
import QtCore
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd

Page {
    id: decompressionPage

    Item {
        width: parent.width   * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            QmlDecompressor { id: decompressor }

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
                              qsTr("Target Archive: ") + decompressor.input :
                              qsTr("No compressed archive selected.")
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    wrapMode: Text.WordWrap
                }
                Button {
                    text: qsTr("Browse Archives...")
                    onClicked: decompressorFilePicker.open()
                }
            }

            Item { Layout.fillHeight: true } // Spacer pushing action button down

            Button {
                text: qsTr("Start Decompression Pipeline")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 250
                enabled: decompressorPrivate.hasFile

                onClicked: {
                    decompressorDialog.text = ""
                    decompressorDialog.informativeText = ""

                    // Single-threaded blocking execution pass
                    if (!decompressor.decompress()) {
                        decompressorDialog.text = qsTr("Error In Decompression")
                        decompressorDialog.informativeText = decompressor.errorString
                    } else {
                        decompressorDialog.text = qsTr("Success")
                        decompressorDialog.informativeText = qsTr("Archive decompressed successfully!")
                    }

                    decompressorDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: decompressorFilePicker
        title: qsTr("Select Zstd Archive for Extraction")
        // currentFolder: StandardPaths.writableLocation(StandardPaths.DownloadLocation)[0]
        currentFolder: StandardPaths.standardLocations(StandardPaths.DownloadLocation)[0]
        nameFilters: [ "Zstd compressed archives (*.zst)", "All files (*)" ]

        onAccepted: {
            let cleanPath = selectedFile.toString();
            if (cleanPath.startsWith("file://")) {
                cleanPath = cleanPath.replace("file://", "");
            }

            // Assign archive to input stream context
            decompressor.input = cleanPath;
            if (cleanPath.endsWith(".zst")) {
                decompressor.output = cleanPath.substring(0, cleanPath.length - 4);
            } else {
                decompressor.output = cleanPath + ".extracted";
            }

            decompressorPrivate.hasFile = true;
        }
    }

    MessageDialog {
        id: decompressorDialog
        buttons: MessageDialog.Ok
    }

    // Private Component Scope Tracking State
    Item {
        id: decompressorPrivate
        property bool hasFile: false
    }
}