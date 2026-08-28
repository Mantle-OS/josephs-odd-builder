import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {
    id: rootHmacSha256Page
    SodiumHmacSha256 { id: hmac }
    Item {
        width: parent.width * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("HMAC Key:")
                    Layout.preferredWidth: 120
                }

                QmlSecureMemInput {
                    id: keyField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    onReturnPressed: {
                        if (hmac.setKey(keyField.memory)) {
                            keyField.secureWipe()
                            hmacDialog.title = qsTr("HMAC Key")
                            hmacDialog.informativeText = qsTr("Key copied into secure memory.")
                            hmacDialog.open()
                        } else {
                            console.error("Secure HMAC key memory copy failed.")
                        }
                    }
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("Message:")
                    Layout.preferredWidth: 120
                }

                TextArea {
                    id: messageField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    placeholderText: qsTr("Message to authenticate")
                    wrapMode: TextEdit.WrapAnywhere
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("Current MAC:")
                    Layout.preferredWidth: 120
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    text: hmac.mac
                    readOnly: true
                    wrapMode: TextEdit.WrapAnywhere
                    selectByMouse: true
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("Previous MAC:")
                    Layout.preferredWidth: 120
                }

                TextArea {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    text: hmac.lastMac
                    readOnly: true
                    wrapMode: TextEdit.WrapAnywhere
                    selectByMouse: true
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true
                Label {
                    text: qsTr("State:")
                    Layout.preferredWidth: 120
                }
                Label { text: hmac.valid ? qsTr("Ready / Valid") : qsTr("Incomplete / Unknown") }
            }

            RowLayout {
                spacing: 12
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: qsTr("Generate Key")

                    onClicked: {
                        const generated = hmac.generateKey()
                        keyField.secureWipe()

                        hmacDialog.title = qsTr("HMAC Key Generation")
                        hmacDialog.informativeText = generated ?
                            qsTr("A new HMAC-SHA256 key was generated directly into secure memory.") :
                            qsTr("Unable to generate a new HMAC-SHA256 key.")
                        hmacDialog.open()
                    }
                }

                Button {
                    text: qsTr("Compute MAC")

                    enabled: hmac.key !== null

                    onClicked: {
                        const result = hmac.compute(messageField.text)

                        hmacDialog.title = qsTr("HMAC-SHA256 Computation")
                        hmacDialog.informativeText = result.length > 0 ?
                            qsTr("HMAC-SHA256 computation completed.") :
                            qsTr("Unable to compute HMAC-SHA256.")
                        hmacDialog.open()
                    }
                }

                Button {
                    text: qsTr("Verify")

                    enabled: hmac.valid

                    onClicked: {
                        const match = hmac.verify(messageField.text)

                        hmacDialog.title = qsTr("HMAC-SHA256 Verification")
                        hmacDialog.informativeText = match ?
                            qsTr("AUTHENTICATED: Message matches the current MAC.") :
                            qsTr("FAILED: Message does not match the current MAC.")
                        hmacDialog.open()
                    }
                }

                Button {
                    text: qsTr("Clear Key")
                    enabled: hmac.key !== null
                    onClicked: {
                        keyField.secureWipe()
                        hmac.clearKey()

                        hmacDialog.title = qsTr("HMAC Key")
                        hmacDialog.informativeText = qsTr("HMAC key buffers cleared.")
                        hmacDialog.open()
                    }
                }
            }
        }
    }

    MessageDialog {
        id: hmacDialog
        buttons: MessageDialog.Ok
    }
}