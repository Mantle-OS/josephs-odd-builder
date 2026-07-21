import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {

    Item {
        width: parent.width * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            RowLayout {
                spacing: 10
                Label { text: qsTr("Raw String Buffer:"); Layout.preferredWidth: 120 }
                TextArea {
                    id: bufferInputField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    placeholderText: qsTr("Type any string or token payload here to compute an instant BLAKE2b hash...")
                }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Target File:"); Layout.preferredWidth: 120 }
                TextField {
                    id: filePathView
                    readOnly: true
                    Layout.fillWidth: true
                    text: QmlSodiumHash.filePath
                    placeholderText: qsTr("Browse for a local file to calculate a hardware checksum...")
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: hashFilePicker.open()
                }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Last Session Hash:"); Layout.preferredWidth: 120 }
                TextField {
                    id: lastHashDisplay
                    Layout.fillWidth: true
                    readOnly: true
                    text: QmlSodiumHash.lastHash
                    placeholderText: qsTr("No hash calculations executed in this session yet.")
                }
            }

            RowLayout {
                spacing: 12
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: qsTr("Hash Text Buffer")
                    onClicked: {
                        hashDialog.title = qsTr("Buffer Processing Wave")
                        let result = QmlSodiumHash.hashBuffer(bufferInputField.text);
                        hashDialog.informativeText  (result.length > 0) ?
                            qsTr("Success! Hex-encoded BLAKE2b Hash:\n\n") + result :
                            qsTr("Aborted: Input text data buffer is empty.");
                        hashDialog.open();
                    }
                }

                Button {
                    text: qsTr("Hash Selected File")
                    onClicked: {
                        hashDialog.title = qsTr("File Stream Processing Wave")
                        if (QmlSodiumHash.filePath.length === 0) {
                            hashDialog.informativeText = qsTr("Error: Please pick a valid target file path first.");
                        } else {
                            let result = QmlSodiumHash.hashFile();
                            hashDialog.informativeText = (result.length > 0) ?
                                qsTr("Success! File Hex-encoded BLAKE2b Hash:\n\n") + result :
                                qsTr("Failed: Check target file visibility or access bounds.")
                        }
                        hashDialog.open();
                    }
                }
            }
        }
    }

    FileDialog {
        id: hashFilePicker
        onAccepted: {
            let pathStr = selectedFile.toString();
            if (pathStr.startsWith("file://"))
                pathStr = Qt.platform.os === "linux" ? pathStr.replace("file://", "") : pathStr.replace("file:///", "")
            QmlSodiumHash.filePath = pathStr;
        }
    }

    MessageDialog {
        id: hashDialog
        buttons: MessageDialog.Ok
    }
}