import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QZstd
import Sodium

Page {
    id: asyncPage

    Connections {
        target: QmlZstd

        function onFinished() {
            if (QmlZstd.errorString === "") {
                asyncDialog.text = qsTr("Pipeline task completed")
                asyncDialog.informativeText = asyncPrivate.isCompressing ?
                            qsTr("Archive generation completed successfully.") :
                            qsTr("Payload extraction completed successfully.")
            } else {
                asyncDialog.text = qsTr("Pipeline task failed")
                asyncDialog.informativeText = QmlZstd.errorString
            }
            asyncDialog.open()
        }
    }

    Item {
        width: parent.width   * 0.85
        height: parent.height * 0.85
        anchors.centerIn: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            Label {
                text: qsTr("Asynchronous pipeline")
                font.pixelSize: 20
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Runs compression or decompression on a background thread, so the window stays responsive the whole time. This is the same underlying engine used everywhere else in this app, wrapped so it never blocks the UI, this is the version a real application would actually use.")
                font.pixelSize: 13
                opacity: 0.75
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20
                RadioButton {
                    id: radioCompress
                    text: qsTr("Compress operations")
                    checked: true
                    onCheckedChanged: if (checked) asyncPrivate.isCompressing = true
                }
                RadioButton {
                    id: radioDecompress
                    text: qsTr("Decompress / extract")
                    onCheckedChanged: if (checked) asyncPrivate.isCompressing = false
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: asyncPrivate.hasSource ?
                              qsTr("Input target: ") + QmlZstd.input :
                              qsTr("Select an input source to begin.")
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                }
                Button {
                    text: qsTr("Browse source")
                    onClicked: asyncSourcePicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: asyncPrivate.isCompressing
                Label { text: qsTr("Compression level (%1):").arg(Math.round(levelSlider.value)) }
                Slider {
                    id: levelSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 22
                    stepSize: 1
                    value: 9
                    onValueChanged: QmlZstd.compressionLevel = Math.round(value)
                }
            }

            GroupBox {
                title: qsTr("Cryptographic settings")
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    CheckBox {
                        id: checkCryptoAction
                        text: asyncPrivate.isCompressing ?
                                  qsTr("Encrypt payload") :
                                  qsTr("Decrypt payload")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: checkCryptoAction.checked
                        Label {
                            text: qsTr("Encryption key:")
                            Layout.preferredWidth: 150
                        }
                        QmlSecureMemInput {
                            id: encryptionKeyInput
                            Layout.fillWidth: true
                            onReturnPressed: {
                                if (!QmlZstd.setEncryptionKey(encryptionKeyInput.memory))
                                    asyncDialog.informativeText = qsTr("Could not set encryption key.")
                                encryptionKeyInput.secureWipe()
                            }
                        }
                    }

                    CheckBox {
                        id: checkSignAction
                        text: asyncPrivate.isCompressing ?
                                  qsTr("Sign archive") :
                                  qsTr("Verify signature")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: checkSignAction.checked
                        Label {
                            text: qsTr("Signing keys:")
                            Layout.preferredWidth: 150
                        }
                        Button {
                            text: qsTr("Generate new keypair")
                            enabled: asyncPrivate.isCompressing
                            onClicked: {
                                if (QmlZstd.signingKeys.create(QmlSodiumKeys.KeyType.Sign)) {
                                    keyDirPicker.open()
                                } else {
                                    asyncDialog.text = qsTr("Key generation failed")
                                    asyncDialog.open()
                                }
                            }
                        }
                        Button {
                            text: qsTr("Load existing keys")
                            onClicked: keyFilePicker.open()
                        }
                        Label {
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                            text: QmlZstd.signingKeys.publicKeyFile !== "" ?
                                      qsTr("Loaded: ") + QmlZstd.signingKeys.publicKeyFile :
                                      qsTr("No signing key loaded")
                            color: Material.color(Material.Grey)
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: asyncPrivate.isCompressing ?
                          qsTr("Dispatch async compressor") :
                          qsTr("Dispatch async decompressor")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 280
                enabled: asyncPrivate.hasSource

                onClicked: {
                    if (asyncPrivate.isCompressing) {
                        QmlZstd.compress(checkSignAction.checked, checkCryptoAction.checked)
                    } else {
                        QmlZstd.decompress(checkSignAction.checked, checkCryptoAction.checked)
                    }
                }
            }
        }
    }

    FileDialog {
        id: asyncSourcePicker
        title: asyncPrivate.isCompressing ?
                   qsTr("Choose target file or folder for packing") :
                   qsTr("Choose Zstd package for unpacking")
        onAccepted: {
            let cleanPath = selectedFile.toString()
            if (cleanPath.startsWith("file://"))
                cleanPath = cleanPath.replace("file://", "")

            QmlZstd.input = cleanPath

            if (asyncPrivate.isCompressing) {
                QmlZstd.output = cleanPath + ".pkg"
            } else if (cleanPath.endsWith(".pkg")) {
                QmlZstd.output = cleanPath.substring(0, cleanPath.length - 4) + ".extracted"
            } else {
                QmlZstd.output = cleanPath + ".extracted"
            }
            asyncPrivate.hasSource = true
        }
    }

    FolderDialog {
        id: keyDirPicker
        title: qsTr("Choose a folder to save the new signing keys")
        onAccepted: {
            let dir = selectedFolder.toString().replace("file://", "")
            QmlZstd.signingKeys.keyDir = dir
            QmlZstd.signingKeys.publicKeyFile = "identity.pub"
            QmlZstd.signingKeys.privateKeyFile = "identity.key"
            if (!QmlZstd.signingKeys.saveKeysToDisk()) {
                asyncDialog.text = qsTr("Failed to save keys")
                asyncDialog.open()
            }
        }
    }

    FileDialog {
        id: keyFilePicker
        title: qsTr("Choose the public key file")
        onAccepted: {
            let path = selectedFile.toString().replace("file://", "")
            QmlZstd.signingKeys.publicKeyFile = path
            // Private key file picking is deliberately left as a second,
            // explicit step rather than guessed from the public key's
            // location, callers should not be silently loading whatever
            // private key file happens to share a directory with the public one.
        }
    }

    MessageDialog {
        id: asyncDialog
        buttons: MessageDialog.Ok
    }

    Item {
        id: asyncPrivate
        property bool isCompressing: true
        property bool hasSource: false
    }
}