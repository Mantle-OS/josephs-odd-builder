import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

// QmlSodiumBox

Page {

    QmlSodiumBox{
        id: box
    }
    Item{
        width: parent.width * 0.80
        height: parent.height * 0.80
        anchors.centerIn: parent
        ColumnLayout{
            anchors.fill: parent
            // QP_RW(QString, password,   "")
            RowLayout{
                Label{text: qsTr("Password")}
                SecureTextField {
                    id: encStrTextField
                    Layout.fillWidth: true
                    onValueCommitted: (clearText) => { box.password = clearText }
                }
            }

            // QP_RW(QString, salt,       "")
            RowLayout{
                Label{text: qsTr("Salt")}
                TextField{
                    id: saltTextField
                    text: box.salt
                    Layout.fillWidth: true
                    onTextChanged:  box.salt = text
                }
            }

            // QP_RW(QString, cipherText, "")
            RowLayout{
                Label{text: qsTr("Cipher Text")}
                TextField{
                    id: cipherTextTextField
                    Layout.fillWidth: true
                    text: box.cipherText
                    onTextChanged: box.cipherText = text
                }
            }

            // QP_RW(QString, nonce,      "")
            RowLayout{
                Label{text: qsTr("nonce")}
                TextField{
                    id: nonceTextField
                    Layout.fillWidth: true
                    text: box.nonce // Updates automatically when encryptString() completes
                    onTextChanged:  box.nonce = text
                }
            }

            // Q_INVOKABLE bool encryptString(const QString &plainText);
            Button{
                text: qsTr("Encrypt")
                Layout.fillWidth: true
                onClicked: {
                    secBoxDialog.title = qsTr("Encryption Action Execution")
                    if (box.encryptString("Text to encrypt testing")) {
                        secBoxDialog.informativeText = qsTr("Encrypt Successful! Values populated to view slots.")
                    } else {
                        secBoxDialog.informativeText = qsTr("Encrypt Failed! Check password/salt inputs.")
                    }
                    secBoxDialog.open();
                }
            }

            // Q_INVOKABLE QString decryptToString();
            Button{
                text: qsTr("Decrypt")
                Layout.fillWidth: true
                onClicked: {
                    secBoxDialog.title = qsTr("Decrypted Output Payload")
                    let decryptedResult = box.decryptToString()

                    if (decryptedResult.length > 0) {
                        secBoxDialog.informativeText = decryptedResult
                    } else {
                        secBoxDialog.informativeText = qsTr("Decryption failed. Authentication MAC mismatch.")
                    }
                    secBoxDialog.open();
                }
            }

            // Q_INVOKABLE void generateNewSalt();
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