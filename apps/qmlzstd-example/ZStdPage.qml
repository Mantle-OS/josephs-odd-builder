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
            asyncDialog.text = qsTr("Pipeline Task Completed")
            if (QmlZstd.errorString === "") {
                asyncDialog.informativeText = asyncPrivate.isCompressing ?
                            qsTr("Asynchronous archive generation completed successfully!") :
                            qsTr("Asynchronous payload extraction completed successfully!")
            } else {
                asyncDialog.text = qsTr("Pipeline Task Failed")
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
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20
                RadioButton {
                    id: radioCompress
                    text: qsTr("Compress Operations")
                    checked: true
                    onCheckedChanged: if (checked) asyncPrivate.isCompressing = true
                }
                RadioButton {
                    id: radioDecompress
                    text: qsTr("Decompress / Extract")
                    onCheckedChanged: if (checked) asyncPrivate.isCompressing = false
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Progress:") }
                ProgressBar {
                    Layout.fillWidth: true
                    value: QmlZstd.current
                    from: 0
                    to: QmlZstd.total > 0 ? QmlZstd.total : 100
                }
                Label {
                    text: QmlZstd.total > 0 ?
                              Math.round((QmlZstd.current / QmlZstd.total) * 100) + "%" :
                              "0%"
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: asyncPrivate.hasSource ?
                            qsTr("Input Target: ") + QmlZstd.input :
                              qsTr("Select an input source to begin...")
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                }
                Button {
                    text: qsTr("Browse Source...")
                    onClicked: asyncSourcePicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: asyncPrivate.isCompressing
                Label { text: qsTr("Compression Level (%1):").arg(Math.round(levelSlider.value)) }
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
                title: qsTr("Cryptographic Authenticated Envelope Settings")
                Layout.fillWidth: true
                visible: QmlZstd.hasSodium

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 8

                    CheckBox {
                        id: checkCryptoAction
                        text: asyncPrivate.isCompressing ?
                                  qsTr("Symmetrically Encrypt Payload Packet") :
                                  qsTr("Decrypt Authenticated SecretBox Payload")
                    }
                    CheckBox {
                        id: checkSignAction
                        text: asyncPrivate.isCompressing ?
                                  qsTr("Generate Standalone Detached Identity Signature") :
                                  qsTr("Enforce Public Key Signature Authentication Validation")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: checkCryptoAction.checked || checkSignAction.checked
                        Label {
                            text: qsTr("Private/Vault Key:")
                            Layout.preferredWidth: 150
                        }

                        SecureTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Base64 Encoded Secret Key String Material")
                            autoWipeOnFocusLoss: true

                            onValueCommitted: function(clearText) {
                                QmlZstd.privateKey = clearText;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: checkSignAction.checked
                        Label {
                            text: qsTr("Public Identity Key:")
                            Layout.preferredWidth: 150
                        }

                        SecureTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Base64 Encoded Public Key Identity String")
                            autoWipeOnFocusLoss: true

                            onValueCommitted: function(clearText) {
                                QmlZstd.publicKey = clearText;
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        visible: asyncPrivate.isCompressing && checkSignAction.checked
                        Label {
                            text: qsTr("Signature Signing Key:")
                            Layout.preferredWidth: 150
                        }

                        SecureTextField {
                            Layout.fillWidth: true
                            placeholderText: qsTr("Base64 Encoded Signature Identity Private Key String")
                            autoWipeOnFocusLoss: true

                            onValueCommitted: function(clearText) {
                                QmlZstd.signatureKey = clearText;
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            Button {
                text: asyncPrivate.isCompressing ?
                          qsTr("Dispatch Async Compressor") :
                          qsTr("Dispatch Async Decompressor")
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 280
                enabled: asyncPrivate.hasSource

                onClicked: {
                    QmlZstd.setErrorString = ""

                    if (asyncPrivate.isCompressing) {
                        if (QmlZstd.hasSodium) {
                            QmlZstd.compress(checkSignAction.checked, checkCryptoAction.checked)
                        } else {
                            QmlZstd.compress()
                        }
                    } else {
                        if (QmlZstd.hasSodium) {
                            QmlZstd.decompress(checkSignAction.checked, checkCryptoAction.checked)
                        } else {
                            QmlZstd.decompress()
                        }
                    }

                    if (QmlZstd.errorString !== "") {
                        asyncDialog.text = qsTr("Pipeline Startup Rejection")
                        asyncDialog.informativeText = QmlZstd.errorString
                        asyncDialog.open()
                    }
                }
            }
        }
    }

    FileDialog {
        id: asyncSourcePicker
        title: asyncPrivate.isCompressing ?
                   qsTr("Choose Target File/Folder for Packing") :
                   qsTr("Choose Zstd Package for Unpacking")
        onAccepted: {
            let cleanPath = selectedFile.toString()
            if (cleanPath.startsWith("file://")) {
                cleanPath = cleanPath.replace("file://", "")
            }

            QmlZstd.input = cleanPath

            if (asyncPrivate.isCompressing) {
                QmlZstd.output = cleanPath + ".pkg"
            } else {
                if (cleanPath.endsWith(".pkg")) {
                    QmlZstd.output = cleanPath.substring(0, cleanPath.length - 4) + ".extracted"
                } else {
                    QmlZstd.output = cleanPath + ".extracted"
                }
            }
            asyncPrivate.hasSource = true
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