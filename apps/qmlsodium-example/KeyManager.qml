import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import Sodium

Page {
    id: rootKeyPage

    QmlSodiumKeys {
        id: keys
        // Set up clean initial tracking filenames for the runtime tests
        publicKeyFile: "identity.pub"
        privateKeyFile: "identity.key"
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
                Label { text: qsTr("Save Folder:") ; Layout.preferredWidth: 100 }
                TextField {
                    id: saveDirView
                    readOnly: true
                    Layout.fillWidth: true
                    text: keys.keyDir
                    placeholderText: "No directory selected..."
                }
                Button {
                    text: qsTr("Browse...")
                    onClicked: dirPicker.open()
                }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Key Type:") ; Layout.preferredWidth: 100 }
                ComboBox {
                    id: typeBox
                    Layout.fillWidth: true
                    model: ["Exchange", "Sign"]
                    // Fixed: Use onActivated to trap genuine drop-down changes instantly
                    onActivated: (index) => {
                        createBtn.keyType = index
                    }
                }
            }

            RowLayout {
                spacing: 10
                Label { text: qsTr("Active Public Key:"); Layout.preferredWidth: 100 }
                TextField {
                    Layout.fillWidth: true
                    readOnly: true
                    text: keys.publicKeyBase64
                    placeholderText: "Keypair not loaded or generated yet."
                }
            }

            RowLayout {
                spacing: 10
                Layout.alignment: Qt.AlignHCenter

                Button {
                    id: createBtn
                    property int keyType: 0
                    text: qsTr("Generate Keypair")
                    onClicked: {
                        signDialog.title = qsTr("Key Generation Matrix")
                        if (keys.create(keyType)) {
                            signDialog.informativeText = qsTr("Keypair successfully minted in RAM.")
                        } else {
                            signDialog.informativeText = qsTr("Generation failed! Enforce init checks.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Save to Disk")
                    onClicked: {
                        signDialog.title = qsTr("Disk Sync Export")
                        if (keys.saveKeysToDisk()) {
                            signDialog.informativeText = qsTr("Keys exported securely onto storage track.")
                        } else {
                            signDialog.informativeText = qsTr("Failed to save keys. Verify dir permissions.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Load From Disk")
                    onClicked: {
                        signDialog.title = qsTr("Disk Sync Import")
                        if (keys.loadKeysFromDisk()) {
                            signDialog.informativeText = qsTr("Keys loaded! Private memory array locked.")
                        } else {
                            signDialog.informativeText = qsTr("Import aborted. Files missing or malformed.")
                        }
                        signDialog.open()
                    }
                }

                Button {
                    text: qsTr("Validate State")
                    onClicked: {
                        signDialog.title = qsTr("Cryptographic Integrity Pass")
                        if (keys.validSet()) {
                            signDialog.informativeText = qsTr("VALID: Active memory blocks match correctly.")
                        } else {
                            signDialog.informativeText = qsTr("INVALID: Key descriptors are empty or disjointed.")
                        }
                        signDialog.open()
                    }
                }
            }
        }
    }


    FolderDialog {
        id: dirPicker
        onAccepted: {
            // Convert file URL schema to clean standard absolute system paths for C++ consumption
            let pathStr = selectedFolder.toString();
            if (pathStr.startsWith("file://")) {
                pathStr = pathStr.replace("file://", "");
            }
            keys.keyDir = pathStr;
        }
    }

    FileDialog {
        id: filePicker
        property string request: "public"
        onAccepted: {
            let pathStr = selectedFile.toString();
            if (pathStr.startsWith("file://")) {
                pathStr = pathStr.replace("file://", "");
            }

            // Fixed: Repaired duplicate conditional statement evaluation check logic
            if (request === "private") {
                keys.privateKeyFile = pathStr
            } else if (request === "public") {
                keys.publicKeyFile = pathStr
            }
        }
    }

    MessageDialog {
        id: signDialog
        buttons: MessageDialog.Ok
    }
}