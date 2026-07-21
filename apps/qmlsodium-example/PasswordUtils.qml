import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {
    id: rootPasswordUtilsPage
    QmlSodiumPasswordUtils { id: passUtils }
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
                    text: qsTr("Password Input:")
                    Layout.preferredWidth: 120
                }

                QmlSecureMemInput {
                    id: passUtilsField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    onReturnPressed: {
                        if (passUtils.setPassword(passUtilsField.memory)) {
                            passUtilsField.secureWipe()
                            utilsDialog.title = qsTr("Password Buffer")
                            utilsDialog.informativeText = qsTr("Password copied into secure memory.")
                            utilsDialog.open()
                        } else {
                            console.error("Secure password memory copy failed.")
                        }
                    }
                }
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Label {
                    text: qsTr("Storage Crypt Hash:")
                    Layout.preferredWidth: 120
                }

                TextArea {
                    id: cryptHashField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    placeholderText: qsTr("Argon2id modular crypt string format ($argon2id$v=19$m=...)")
                    wrapMode: TextEdit.WrapAnywhere
                }
            }

            RowLayout {
                spacing: 12
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: qsTr("Hash for Storage")
                    onClicked: {
                        utilsDialog.title = qsTr("Argon2id Hash Routine")
                        const resultStr = passUtils.hashForStorage()
                        if (resultStr.length > 0) {
                            cryptHashField.text = resultStr
                            passUtils.clearPassword()
                            utilsDialog.informativeText = qsTr("Success: Password securely salted and processed. Crypt string output populated below.")
                        } else {
                            utilsDialog.informativeText = qsTr("Aborted: Current password buffer is empty.")
                        }
                        utilsDialog.open()
                    }
                }

                Button {
                    text: qsTr("Verify Against Storage")
                    onClicked: {
                        utilsDialog.title = qsTr("Authentication Match Pass")
                        if (cryptHashField.text.length === 0) {
                            utilsDialog.informativeText = qsTr("Error: Please provide a valid storage crypt string to match against.")
                        } else {
                            const match = passUtils.verifyAgainstStorage(cryptHashField.text)
                            passUtils.clearPassword()
                            utilsDialog.informativeText =  match ?
                                qsTr("ACCESS GRANTED: Password matches target Argon2id parameters.") :
                                qsTr("ACCESS DENIED: Password mismatch or storage string is malformed.")
                        }
                        utilsDialog.open()
                    }
                }

                Button {
                    text: qsTr("Clear Password")
                    onClicked: {
                        passUtilsField.secureWipe()
                        passUtils.clearPassword()
                        //
                        utilsDialog.title = qsTr("Password Buffer")
                        utilsDialog.informativeText = qsTr("Password buffers cleared.")
                        utilsDialog.open()
                    }
                }
            }
        }
    }

    MessageDialog {
        id: utilsDialog
        buttons: MessageDialog.Ok
    }
}