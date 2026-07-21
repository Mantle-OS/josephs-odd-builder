import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

// QmlSodiumCryptoSign Example
Page {
    QmlSodiumCryptoSign { id: signer }

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
                    placeholderText: qsTr("Select a file to sign or verify...")
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: {
                        fileToSignPicker.fileToSign = 0
                        fileToSignPicker.open()
                    }
                }
            }

            RowLayout {
                spacing: 10
                Label {
                    id: publicKeyLabel
                    property string publicKeyFilePath: ""
                    text: qsTr("Public Key File: ") + publicKeyFilePath ;
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Open Public Key File...")
                    onClicked: {
                        fileToSignPicker.fileToSign = 1
                        fileToSignPicker.open()
                    }
                }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                spacing: 10
                Label {
                    id: privateKeyLabel
                    property string privateKeyFilePath: ""
                    text: qsTr("Private Key File: ") + privateKeyFilePath ;
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Open Private Key File...")
                    onClicked: {
                        fileToSignPicker.fileToSign = 2
                        fileToSignPicker.open()
                    }
                }
                Item { Layout.fillWidth: true }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Signature (B64):"); Layout.preferredWidth: 120 }
                TextField {
                    id: sigField
                    Layout.fillWidth: true
                    text: signer.signatureBase64
                    placeholderText: qsTr("Calculated signature or verification token...")
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
                        if(!signer.hasKeys())
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        else
                            signDialog.informativeText =  signer.signFile() ?
                                qsTr("Success: Detached signature updated.") :
                                qsTr("Failed: Check key context and file location.")
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Sign Associated File")
                    onClicked: {
                        signDialog.title = qsTr("Associated Key Sync Pass")
                        if(!signer.hasKeys())
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        else
                            signDialog.informativeText = signer.signAssociatedFile() ?
                                qsTr("Success: Signature verified via binding.") :
                                qsTr("Failed: Action signature mismatch.")
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Verify Signature")
                    onClicked: {
                        signDialog.title = qsTr("Cryptographic Verification")
                        if(!signer.hasKeys())
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        else
                            signDialog.informativeText = signer.verifyAssociatedFile() ?
                                qsTr("MATCH: Manifest verification successful.") :
                                qsTr("REJECTED: Signature invalid or file altered.")
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Compute BLAKE2b")
                    onClicked: {
                        signDialog.title = qsTr("Hardware Checksum Compute")
                        let hashResult = signer.computeFileBlake2b()
                        signDialog.informativeText =  hashResult.length > 0 ?
                                    qsTr("BLAKE2b Checksum:\n") + hashResult :
                                    qsTr("Hashed pass loop dropped.")
                        signDialog.open()
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileToSignPicker
        property int fileToSign: 0 // 0 = file to sign | 1 = pub_key | 2 = pri_key
        onAccepted: {
            let pathStr = selectedFile.toString()
            if (fileToSign == 0)
                signer.update_filePath(selectedFile)
            else if (fileToSign == 1)
                publicKeyLabel.publicKeyFilePath = signer.update_publicKey(selectedFile) ? pathStr : qsTr("Unknown file")
            else if (fileToSign == 2)
                privateKeyLabel.privateKeyFilePath = signer.update_privateKey(selectedFile) ? pathStr : qsTr("Unknown file")
        }
    }

    MessageDialog {
        id: signDialog
        buttons: MessageDialog.Ok
    }
}