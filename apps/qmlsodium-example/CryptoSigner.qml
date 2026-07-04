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
                Item { Layout.fillWidth: true } // Spacer
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
                Item { Layout.fillWidth: true } // Spacer
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
                        if(!signer.hasKeys()){
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        } else {
                            signer.signFile() ?
                                signDialog.informativeText = qsTr("Success: Detached signature updated.") :
                                signDialog.informativeText = qsTr("Failed: Check key context and file location.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Sign Associated File")
                    onClicked: {
                        signDialog.title = qsTr("Associated Key Sync Pass")
                        if(!signer.hasKeys()){
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        } else {
                            signer.signAssociatedFile() ?
                                signDialog.informativeText = qsTr("Success: Signature verified via binding.") :
                                signDialog.informativeText = qsTr("Failed: Action signature mismatch.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Verify Signature")
                    onClicked: {
                        signDialog.title = qsTr("Cryptographic Verification")
                        if(!signer.hasKeys()){
                            signDialog.informativeText = qsTr("Failure: Could not load keys. make sure that you have picked out a public and private key")
                        }else{
                            signer.verifyAssociatedFile() ?
                                signDialog.informativeText = qsTr("MATCH: Manifest verification successful.") :
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
                        hashResult.length > 0 ?
                                    signDialog.informativeText = qsTr("BLAKE2b Checksum:\n") + hashResult :
                                    signDialog.informativeText = qsTr("Hashed pass loop dropped.")
                        signDialog.open()
                    }
                }
            }
        }
    }

    FileDialog {
        id: fileToSignPicker
        property int fileToSign: 0 //  0 = file to sign | 1 = pub key | 2 = pri key

        onAccepted: {
            let pathStr = selectedFile.toString()
            if (pathStr.startsWith("file://"))
                pathStr = pathStr.replace("file://", "")

            if (fileToSign == 0) {
                signer.filePath = pathStr
            } else if (fileToSign == 1){
                signer.publicKey = pathStr;
                publicKeyLabel.publicKeyFilePath = pathStr
            } else if (fileToSign == 2){
                privateKeyLabel.privateKeyFilePath = pathStr
                signer.privateKey = pathStr;
            }
        }
    }

    MessageDialog {
        id: signDialog
        buttons: MessageDialog.Ok
    }
}