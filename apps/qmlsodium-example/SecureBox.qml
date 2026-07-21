import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

// QmlSodiumBox Example
Page {
    QmlSodiumBox{ id: box }
    Item{
        width: parent.width * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent
        ColumnLayout {
            anchors.fill: parent
            RowLayout{
                Label{text: qsTr("Password")}
                QmlSecureMemInput{
                    id: encStrTextField
                    Layout.fillWidth: true
                    onReturnPressed: box.setPassword(encStrTextField.memory) ?
                                         encStrTextField.secureWipe() :
                                         console.error("Secure password memory copy failed.")
                }
            }

            RowLayout{
                Label{text: qsTr("Salt")}
                TextField{
                    id: saltTextField
                    text: box.salt
                    Layout.fillWidth: true
                    onTextChanged:  box.salt = text
                }
            }

            RowLayout{
                Label{text: qsTr("Cipher Text")}
                TextField{
                    id: cipherTextTextField
                    Layout.fillWidth: true
                    text: box.cipherText
                    onTextChanged: box.cipherText = text
                }
            }

            RowLayout{
                Label{text: qsTr("Nonce")}
                TextField{
                    id: nonceTextField
                    Layout.fillWidth: true
                    text: box.nonce
                    onTextChanged:  box.nonce = text
                }
            }

            Button{
                text: qsTr("Encrypt")
                Layout.fillWidth: true
                onClicked: {
                    secBoxDialog.title = qsTr("Encryption Action Execution")
                    secBoxDialog.informativeText = box.encryptString("Text to encrypt testing") ?
                                qsTr("Encrypt Successful! Values populated to view slots.") :
                                qsTr("Encrypt Failed! Check password/salt inputs.")
                    secBoxDialog.open();
                }
            }

            Button{
                text: qsTr("Decrypt")
                Layout.fillWidth: true
                onClicked: {
                    secBoxDialog.title = qsTr("Decrypted Output Payload")
                    let decryptedResult = box.decryptToString()
                    secBoxDialog.informativeText = (decryptedResult.length > 0) ?
                                decryptedResult :
                                qsTr("Decryption failed. Authentication MAC mismatch.")
                    secBoxDialog.open();
                }
            }

            Button{
                text: qsTr("Generate New Salt")
                Layout.fillWidth: true
                onClicked:  box.generateNewSalt();
            }
        }

        MessageDialog{
            id: secBoxDialog
            buttons: MessageDialog.Ok
            onButtonClicked: close()
        }
    }
}