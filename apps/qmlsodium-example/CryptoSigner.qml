import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {
    id: rootSignerPage

    QmlSodiumCryptoSign {
        id: signer
    }

    Item {
        width: parent.width * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 15

            RowLayout {
                spacing: 10
                Label { text: qsTr("Target File:"); Layout.preferredWidth: 120 }
                TextField {
                    id: filePathView
                    readOnly: true
                    Layout.fillWidth: true
                    text: signer.filePath
                    placeholderText: "Select a file to sign or verify..."
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: {
                        fileToSignPicker.fileToSign = true
                        fileToSignPicker.open()
                    }
                }
            }

            RowLayout {
                spacing: 10
                Label {
                    id: publicKeyLabel
                    property string publicKeyFilePath: ""
                    text: qsTr("Public Key (B64): ") + publicKeyFilePath ;
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Open Public Key File...")
                    onClicked: {
                        fileToSignPicker.fileToSign = false
                        fileToSignPicker.open()
                    }
                }
                Item { Layout.fillWidth: true } // Spacer
            }

            TextArea {
                id: pubKeyField
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                text: signer.publicKeyBase64
                placeholderText: "Paste key or load key data from file above..."
                onTextChanged: signer.publicKeyBase64 = text
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Signature (B64):"); Layout.preferredWidth: 120 }
                TextField {
                    id: sigField
                    Layout.fillWidth: true
                    text: signer.signatureBase64
                    placeholderText: "Calculated signature or verification token..."
                    onTextChanged: signer.signatureBase64 = text
                }
            }

            RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: qsTr("Sign File")
                    onClicked: {
                        signDialog.title = qsTr("Signature Computation Pass")
                        if (signer.signFile()) {
                            signDialog.informativeText = qsTr("Success: Detached signature updated.")
                        } else {
                            signDialog.informativeText = qsTr("Failed: Check key context and file location.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Sign Associated File")
                    onClicked: {
                        signDialog.title = qsTr("Associated Key Sync Pass")
                        if (signer.signAssociatedFile()) {
                            signDialog.informativeText = qsTr("Success: Signature verified via binding.")
                        } else {
                            signDialog.informativeText = qsTr("Failed: Action signature mismatch.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Verify Signature")
                    onClicked: {
                        signDialog.title = qsTr("Cryptographic Verification")
                        if (signer.verifyAssociatedFile()) {
                            signDialog.informativeText = qsTr("MATCH: Manifest verification successful.")
                        } else {
                            signDialog.informativeText = qsTr("REJECTED: Signature invalid or file altered.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Compute BLAKE2b")
                    onClicked: {
                        signDialog.title = qsTr("Hardware Checksum Compute")
                        let hashResult = signer.computeFileBlake2b()
                        if (hashResult.length > 0) {
                            signDialog.informativeText = qsTr("BLAKE2b Checksum:\n") + hashResult
                        } else {
                            signDialog.informativeText = qsTr("Hashed pass loop dropped.")
                        }
                        signDialog.open()
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileToSignPicker
        property bool fileToSign: true

        onAccepted: {
            let pathStr = selectedFile.toString()
            if (pathStr.startsWith("file://")) {
                pathStr = pathStr.replace("file://", "")
            }

            if (fileToSign) {
                signer.filePath = pathStr
            } else {
                // Pipe the string straight to C++ and let the property notifications handle the UI text dump
                if (!signer.loadPublicKeyFromFile(pathStr)) {
                    signDialog.title = qsTr("File Load Error")
                    signDialog.informativeText = qsTr("Could not read public key text from target file.")
                    signDialog.open()
                }
                publicKeyLabel.publicKeyFilePath = pathStr

            }
        }
    }

    MessageDialog {
        id: signDialog
        buttons: MessageDialog.Ok
    }
}