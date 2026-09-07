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
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: 18
                font.weight: Font.Medium
                text: qsTr("SHA Utilities — generate hashes for small in-memory data. Do not use this page for hashing files or large payloads; use the Hash page for streaming file hashing.")
            }
            RowLayout {
                Label {
                    text: qsTr("Input String:")
                    Layout.preferredWidth: parent.width / 8
                }
                TextField {
                    id: inputField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Enter text to hash...")
                }
            }

            RowLayout {
                Label {
                    text: qsTr("SHA Type:")
                    Layout.preferredWidth: parent.width / 8
                }
                ComboBox {
                    Layout.fillWidth: true
                    model: SodiumSha.shaTypes
                    currentIndex: SodiumSha.currentType
                    textRole: "display"
                    onActivated: SodiumSha.currentType = currentIndex
                }
            }

            RowLayout {
                Label {
                    text: qsTr("Signal Output:")
                    Layout.preferredWidth: parent.width / 8
                }
                TextField {
                    id: signalOutput
                    Layout.fillWidth: true
                    readOnly: true
                    placeholderText: qsTr("Waiting for SodiumSha.finished...")
                }
            }

            Label {
                Layout.fillWidth: true
                text: SodiumSha.lastSha === "unknown" ?
                          qsTr("Last SHA:") :
                          qsTr("Last SHA: ") + SodiumSha.lastSha
                wrapMode: Text.WrapAnywhere
            }

            Button {
                text: qsTr("Generate SHA")
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                enabled: inputField.text.trim().length > 0
                onClicked: {
                    shaDialog.title = qsTr("SHA Calculation")
                    let result = SodiumSha.computeHex(inputField.text)
                    shaDialog.informativeText = result.length > 0 ?
                                                   qsTr("SHA generated successfully.") :
                                                   SodiumSha.errorString
                    shaDialog.open()
                }
            }
        }
    }

    Item {
        // im private now
        Connections {
            target: SodiumSha
            function onFinished(type, sha) {
                signalOutput.text = sha
            }
        }
    }

    MessageDialog {
        id: shaDialog
        buttons: MessageDialog.Ok
    }
}