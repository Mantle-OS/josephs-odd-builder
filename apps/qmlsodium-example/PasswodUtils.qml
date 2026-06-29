import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {
    id: rootPasswordUtilsPage

    QmlSodiumPasswordUtils {
        id: pwdUtils
        // QP_RW(QString, password, "")
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
                Label { text: qsTr("Password Input:"); Layout.preferredWidth: 120 }
                SecureTextField {
                    id: passwordInput
                    Layout.fillWidth: true
                    // placeholderText: "Type password entropy string..."

                    onValueCommitted: (clearText) => {
                        pwdUtils.password = clearText
                    }
                }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Storage Crypt Hash:"); Layout.preferredWidth: 120 }
                TextArea {
                    id: cryptHashField
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    placeholderText: "Argon2id modular crypt string format ($argon2id$v=19$m=...)"
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

                        // Compute using the state currently bound to the backend
                        let resultStr = pwdUtils.hashForStorage();

                        if (resultStr.length > 0) {
                            cryptHashField.text = resultStr;
                            utilsDialog.informativeText = qsTr("Success: Password securely salted and processed. Crypt string output populated below.");
                        } else {
                            utilsDialog.informativeText = qsTr("Aborted: Current password buffer property is empty.");
                        }
                        utilsDialog.open();
                    }
                }

                Button {
                    text: qsTr("Verify Against Storage")
                    onClicked: {
                        utilsDialog.title = qsTr("Authentication Match Pass")

                        if (cryptHashField.text.length === 0) {
                            utilsDialog.informativeText = qsTr("Error: Please provide a valid storage crypt string to match against.");
                        } else {
                            // Run deep verification check via C++ stack
                            let match = pwdUtils.verifyAgainstStorage(cryptHashField.text);

                            if (match) {
                                utilsDialog.informativeText = qsTr("ACCESS GRANTED: Password entropy safely matches target Argon2id parameters.");
                            } else {
                                utilsDialog.informativeText = qsTr("ACCESS DENIED: Password mismatch or storage string is malformed.");
                            }
                        }
                        utilsDialog.open();
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