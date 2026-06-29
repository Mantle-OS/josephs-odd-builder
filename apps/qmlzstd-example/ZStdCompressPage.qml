import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd

Page {
    id: compressionPage

    Item {
        width: parent.width   * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            QmlCompressor {
                id: compressor
                // Bindings listen directly to these properties
                compressionLevel: Math.round(levelSlider.value)

                onFinished: {
                    if (compressor.errorString === "") {
                        compressorDialog.text = qsTr("Success")
                        compressorDialog.informativeText = qsTr("Package compiled successfully!")
                    } else {
                        compressorDialog.text = qsTr("Error Encountered")
                        compressorDialog.informativeText = compressor.errorString
                    }
                    compressorDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Progression:") }

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
                Label { text: qsTr("Compression Level (%1):").arg(Math.round(levelSlider.value)) }
                Slider {
                    id: levelSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 22
                    stepSize: 1
                    value: 5
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: qsTr("Start Compression Pipeline")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 250
                enabled: compressorPrivate.hasFolder && (compressor.current === 0 || compressor.current === compressor.total)
                onClicked: {
                    compressorDialog.text = ""
                    compressorDialog.informativeText = ""

                    // Fire up the concurrent executor job
                    if(!compressor.compress()){
                        compressorDialog.text = qsTr("Error In Compression")
                        compressorDialog.informativeText = compressor.errorString
                    }else{
                        compressorDialog.text = qsTr("Compression Passed")
                        compressorDialog.informativeText = qsTr("Package compiled successfully!")
                    }
                    compressorDialog.open()
                }
            }
        }
    }

    FolderDialog {
        id: compressorFolderPicker
        title: qsTr("Select Input Target Directory")
        onAccepted: {
            // Convert file:// URL string layout into clear system native paths
            let cleanPath = selectedFolder.toString();
            if (cleanPath.startsWith("file://")) {
                cleanPath = cleanPath.replace("file://", "");
            }

            // Set input tracking target
            compressor.input = cleanPath;

            // Derive output payload companion asset name target seamlessly
            compressor.output = cleanPath + ".zst";

            compressorPrivate.hasFolder = true;
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
}