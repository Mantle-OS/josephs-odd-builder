import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd
import Sodium

Page {
    QmlCyptoCompressor { id: encryptor }
    QmlCryptoDecompressor { id: decryptor }

    Item {
        width: parent.width * 0.85
        height: parent.height * 0.85
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 20

            Label {
                text: qsTr("Password based encryption")
                font.pixelSize: 20
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("This page compresses and encrypts a file or folder using a password, then decrypts and extracts it back. The password is never held as plain text, it is typed directly into secure memory. A random salt is generated the first time you encrypt, and that exact salt has to be copied into the decrypt side below before decryption can succeed, the same password with a different salt produces a completely different key.")
                font.pixelSize: 13
                opacity: 0.75
            }

            GroupBox {
                title: qsTr("Compress and encrypt")
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            text: cryptoPrivate.encryptSource !== "" ?
                                      cryptoPrivate.encryptSource :
                                      qsTr("No file or folder selected.")
                        }
                        Button {
                            text: qsTr("Folder...")
                            onClicked: {
                                encryptPicker.forDecrypt = false
                                encryptFolderPicker.open()
                            }
                        }
                        Button {
                            text: qsTr("File...")
                            onClicked: {
                                encryptPicker.forDecrypt = false
                                encryptFilePicker.open()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: encryptor.salt !== ""
                        Label { text: qsTr("Salt (copy this to decrypt later):"); Layout.preferredWidth: 220 }
                        TextField {
                            Layout.fillWidth: true
                            readOnly: true
                            text: encryptor.salt
                            selectByMouse: true
                        }
                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Compress and encrypt")
                        enabled: cryptoPrivate.encryptSource !== ""
                        onClicked:  encryptPasswordPopup.open()
                    }
                }
            }

            GroupBox {
                title: qsTr("Decrypt and decompress")
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            text: cryptoPrivate.decryptSource !== "" ?
                                      cryptoPrivate.decryptSource :
                                      qsTr("No archive selected.")
                        }
                        Button {
                            text: qsTr("Folder...")
                            onClicked: {
                                encryptPicker.forDecrypt = true
                                decryptFolderPicker.open()
                            }
                        }
                        Button {
                            text: qsTr("File...")
                            onClicked: {
                                encryptPicker.forDecrypt = true
                                decryptFilePicker.open()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Salt (from the encrypt step above):"); Layout.preferredWidth: 220 }
                        TextField {
                            id: decryptSaltField
                            Layout.fillWidth: true
                            selectByMouse: true
                            onTextChanged: decryptor.salt = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Button {
                            text: qsTr("Options...")
                            onClicked: optionsPopup.open()
                        }
                    }

                    Button {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Decrypt and decompress")
                        enabled: cryptoPrivate.decryptSource !== "" && decryptor.salt !== ""
                        onClicked:  decryptPasswordPopup.open()
                    }
                }
            }
        }
    }

    Popup {
        id: encryptPasswordPopup
        modal: true
        focus: true
        width: 420
        height: 160
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2


        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label { text: qsTr("Enter the password to encrypt with:") }
            QmlSecureMemInput {
                id: encryptPasswordInput
                Layout.fillWidth: true
                onReturnPressed: encryptConfirmDialog.open()
            }
            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Continue")
                onClicked: encryptConfirmDialog.open()
            }
        }
    }

    MessageDialog {
        id: encryptConfirmDialog
        title: qsTr("Confirm encryption")
        text: qsTr("Compress and encrypt \"%1\"? This cannot be undone.").arg(cryptoPrivate.encryptSource)
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (!encryptor.setPassword(encryptPasswordInput.memory)) {
                resultDialog.text = qsTr("Could not set password.")
                resultDialog.open()
            } else {
                encryptPasswordInput.secureWipe()
                encryptPasswordPopup.close()

                resultDialog.text = encryptor.compress() ?
                            qsTr("Compression and encryption complete.") :
                            qsTr("Failed: ") + encryptor.errorString

                resultDialog.open()
            }
        }
    }

    Popup {
        id: decryptPasswordPopup
        modal: true
        focus: true
        width: 420
        height: 160
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Label { text: qsTr("Enter the password to decrypt with:") }
            QmlSecureMemInput {
                id: decryptPasswordInput
                Layout.fillWidth: true
                onReturnPressed: decryptConfirmDialog.open()
            }
            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Continue")
                onClicked: decryptConfirmDialog.open()
            }
        }
    }

    MessageDialog {
        id: decryptConfirmDialog
        title: qsTr("Confirm decryption")
        text: qsTr("Decrypt and decompress \"%1\"?").arg(cryptoPrivate.decryptSource)
        buttons: MessageDialog.Yes | MessageDialog.No
        onAccepted: {
            if (!decryptor.setPassword(decryptPasswordInput.memory)) {
                resultDialog.text = qsTr("Could not set password.")
                resultDialog.open()
            } else {
                decryptPasswordInput.secureWipe()
                decryptPasswordPopup.close()
                resultDialog.text = decryptor.decompress() ?
                            qsTr("Decryption and decompression complete.") :
                            qsTr("Failed: ") + decryptor.errorString
                resultDialog.open()
            }
        }
    }

    FolderDialog {
        id: encryptFolderPicker
        title: qsTr("Select a folder to encrypt")
        onAccepted: {
            let p = selectedFolder.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            encryptor.input = p
            encryptor.output = p + ".zst.enc"
            cryptoPrivate.encryptSource = p
        }
    }

    FileDialog {
        id: encryptFilePicker
        title: qsTr("Select a file to encrypt")
        onAccepted: {
            let p = selectedFile.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            encryptor.input = p
            encryptor.output = p + ".zst.enc"
            cryptoPrivate.encryptSource = p
        }
    }

    FolderDialog {
        id: decryptFolderPicker
        title: qsTr("Select an encrypted archive")
        onAccepted: {
            let p = selectedFolder.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            decryptor.input = p
            decryptor.output = p + ".extracted"
            cryptoPrivate.decryptSource = p
        }
    }

    FileDialog {
        id: decryptFilePicker
        title: qsTr("Select an encrypted archive")
        onAccepted: {
            let p = selectedFile.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            decryptor.input = p
            decryptor.output = p + ".extracted"
            cryptoPrivate.decryptSource = p
        }
    }

    MessageDialog {
        id: resultDialog
        buttons: MessageDialog.Ok
    }

    Item {
        id: cryptoPrivate
        property string encryptSource: ""
        property string decryptSource: ""
    }

    QtObject {
        id: encryptPicker
        property bool forDecrypt: false
    }

    ZStdOptionsPopup {
        id: optionsPopup
        target: decryptor
        width: parent.width * 0.98
        height: parent.height * 0.98
        anchors.centerIn: parent
    }
}
