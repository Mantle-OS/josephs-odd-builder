import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QSd

Pane {
    Layout.fillWidth: true
    Layout.fillHeight: true
    padding: 8

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Z-Image Turbo")
            font.bold: true
        }

        MenuSeparator { Layout.fillWidth: true }
        GroupBox {
            title: qsTr("Model Files")

            Layout.fillWidth: true

            GridLayout {
                anchors.fill: parent
                columns: 3
                columnSpacing: 8
                rowSpacing: 8

                Label { text: qsTr("Diffusion model:") }
                TextField {
                    Layout.fillWidth: true
                    readOnly: true
                    text: zImagePrivate.diffusionModelPath
                    placeholderText: qsTr("Select the diffusion model")
                }

                Button {
                    text: qsTr("Choose…")
                    onClicked:  zImagePrivate.openFilePicker( zImagePrivate.diffusionModelFile )
                }

                Label { text: qsTr("LLM:") }
                TextField {
                    Layout.fillWidth: true
                    readOnly: true
                    text: zImagePrivate.llmPath
                    placeholderText: qsTr("Select the text encoder")
                }

                Button {
                    text: qsTr("Choose…")
                    onClicked:  zImagePrivate.openFilePicker(zImagePrivate.llmFile)
                }

                Label { text: qsTr("VAE:") }
                TextField {
                    Layout.fillWidth: true
                    readOnly: true
                    text: zImagePrivate.vaePath
                    placeholderText: qsTr("Select the VAE")
                }

                Button {
                    text: qsTr("Choose…")
                    onClicked: zImagePrivate.openFilePicker(zImagePrivate.vaeFile)
                }
            }
        }

        // Loras
        // so we have access to the model of the loras via QSD.ImageGenerationParams.loras  we also already have a QmlSdLora.qml
        // We should have a button "LoRa's" that opens up a Popup that has the Lora  MVC in it
        // The popup should also have Add, Remove
        // add file picker to add a lora.  after added append to the MVC and it updates the Gui
        // remove you can littlerly rome the whole object from ths MVC




        Label {
            text: qsTr("Prompt")
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 100
            clip: true
            TextArea {
                id: prompt
                text: "a lovely cat holding a sign says QSD Z-Image Turbo"
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                placeholderText: qsTr("Describe the image to generate")
            }
        }

        GroupBox {
            title: qsTr("Generation Settings")
            Layout.fillWidth: true
            GridLayout {
                anchors.fill: parent
                columns: 4
                columnSpacing: 12
                rowSpacing: 8

                Label { text: qsTr("Width:") }
                SpinBox {
                    id: widthSpinBox
                    from: 64
                    to: 4096
                    stepSize: 64
                    value: 1024
                    editable: true
                }

                Label { text: qsTr("Height:") }
                SpinBox {
                    id: heightSpinBox
                    from: 64
                    to: 4096
                    stepSize: 64
                    value: 1024
                    editable: true
                }

                Label { text: qsTr("Seed:") }
                SpinBox {
                    id: seedSpinBox

                    from: -1
                    to: 2147483647
                    value: 42
                    editable: true
                }

                Label { text: qsTr("Steps:") }
                SpinBox {
                    id: stepsSpinBox
                    from: 1
                    to: 100
                    value: 8
                    editable: true
                }

                Label { text: qsTr("Text CFG:") }
                TextField {
                    id: textCfgField
                    text: "1.0"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator {
                        bottom: 0.0
                        top: 100.0
                        decimals: 4
                    }
                }

                Label { text: qsTr("Flow shift:") }



                TextField {
                    text: "1.0"
                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                    validator: DoubleValidator {
                        bottom: -100.0
                        top: 100.0
                        decimals: 4
                    }
                    onAccepted:  QSD.ImageGenerationParams.sampleParams.flowShift = Number(text)
                }

                Switch {
                    id: flashAttentionSwitch
                    Layout.columnSpan: 2
                    text: qsTr("Diffusion flash attention")
                    checked: true
                }

                Switch {
                    id: weightsOnCpuSwitch
                    Layout.columnSpan: 2
                    text: qsTr("Weights on CPU")
                    checked: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Switch {
                id: autoSaveSwitch
                text: qsTr("Save file")
                checked: false
            }

            Item { Layout.fillWidth: true }

            Button {
                text: qsTr("Generate Image")
                onClicked: {
                    if (!zImagePrivate.validateConfiguration())
                        return

                    QSD.ContextParams.diffusionModelPath =
                        zImagePrivate.diffusionModelPath

                    QSD.ContextParams.llmPath =
                        zImagePrivate.llmPath

                    QSD.ContextParams.vaePath =
                        zImagePrivate.vaePath

                    QSD.ContextParams.diffusionFlashAttn =
                        flashAttentionSwitch.checked

                    QSD.ContextParams.weightsOnCpu =
                        weightsOnCpuSwitch.checked

                    QSD.ImageGenerationParams.imgWidth = widthSpinBox.value

                    QSD.ImageGenerationParams.imgHeight = heightSpinBox.value

                    QSD.ImageGenerationParams.seed = seedSpinBox.value

                    QSD.ImageGenerationParams.prompt = prompt.text

                    QSD.ImageGenerationParams
                        .sampleParams
                        .sampleSteps = stepsSpinBox.value

                    QSD.ImageGenerationParams
                        .sampleParams
                        .guidance
                        .txtCfg = Number(textCfgField.text)

                    QSD.ImageGenerationParams
                        .sampleParams
                        .flowShift = Number(flowShiftField.text)

                    imagePopup.open()

                    QSD.generateImage(
                        outputImage,
                        autoSaveSwitch.checked
                    )
                }
            }
        }
    }

    Popup {
        id: imagePopup
        parent: Overlay.overlay
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        width: Math.min(
            Overlay.overlay.width - 32,
            widthSpinBox.value + padding * 2
        )
        height: Math.min(
            Overlay.overlay.height - 32,
            heightSpinBox.value +
                popupHeader.implicitHeight +
                padding * 2 +
                8
        )
        x: Math.round( (Overlay.overlay.width - width) / 2 )
        y: Math.round( (Overlay.overlay.height - height) / 2 )

        padding: 8

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            RowLayout {
                id: popupHeader

                Layout.fillWidth: true

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Generated Image")
                    font.bold: true
                }

                Button {
                    text: qsTr("Close")
                    onClicked: imagePopup.close()
                }
            }

            // BusyIndicator{
            //     id: isGenerating
            //     running: true
            //     Layout.fillWidth: false
            //     Layout.fillHeight: true
            //     Layout.preferredHeight: parent.width / 2
            // }

            QSdImage {
                id: outputImage
                visible: !isGenerating.running
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            // Connections {
            //     target: QSD
            //     function onStateChanged(state) {
            //         switch (state) {
            //         case QSD.Starting:
            //         case QSD.Running:
            //             isGenerating.running = true
            //             break
            //         case QSD.Finished:
            //             isGenerating.running = false
            //             break
            //         }
            //     }
            // }
        }
    }

    FileDialog {
        id: zImageFilePicker
        title: qsTr("Select Model File")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("Model files (*.gguf *.safetensors *.ckpt)"),
            qsTr("All files (*)")
        ]
        onAccepted: zImagePrivate.acceptSelectedFile(selectedFile)
    }

    MessageDialog {
        id: zImageErrorDialog
        title: qsTr("Z-Image Configuration Error")
        buttons: MessageDialog.Ok
    }

    Item {
        id: zImagePrivate

        visible: false

        readonly property int diffusionModelFile: 0
        readonly property int llmFile: 1
        readonly property int vaeFile: 2
        property int selectedFileType: diffusionModelFile
        property string diffusionModelPath: "/srv/ai/ComfyUI/models/unet/ZImage_Turbo/z_image_turbo-Q8_0.gguf"
        property string llmPath: "/srv/ai/ComfyUI/models/text_encoders/ZImage_Turbo/Qwen3-4B-Instruct-2507-Q4_K_M.gguf"
        property string vaePath: "/srv/ai/ComfyUI/models/vae/ZImage_Turbo/whatever.safetensors"

        function openFilePicker(fileType) {
            selectedFileType = fileType

            switch (fileType) {
            case diffusionModelFile:
                zImageFilePicker.title =
                    qsTr("Select Diffusion Model")
                break

            case llmFile:
                zImageFilePicker.title =
                    qsTr("Select LLM")
                break

            case vaeFile:
                zImageFilePicker.title =
                    qsTr("Select VAE")
                break
            }

            zImageFilePicker.open()
        }

        function acceptSelectedFile(fileUrl) {
            let path = decodeURIComponent(
                fileUrl.toString()
            )

            if (path.startsWith("file://"))
                path = path.substring(7)

            switch (selectedFileType) {
            case diffusionModelFile:
                diffusionModelPath = path
                break

            case llmFile:
                llmPath = path
                break

            case vaeFile:
                vaePath = path
                break
            }
        }

        function validateConfiguration() {
            let error = ""
            if (diffusionModelPath.length === 0) {
                error = qsTr( "Select a diffusion model." )
            } else if (llmPath.length === 0) {
                error = qsTr("Select an LLM.")
            } else if (vaePath.length === 0) {
                error = qsTr( "Select a VAE." )
            } else if (prompt.text.trim().length === 0) {
                error = qsTr( "Enter an image prompt." )
            } else if (!textCfgField.acceptableInput) {
                error = qsTr( "Text CFG is not valid." )
            } else if (!flowShiftField.acceptableInput) {
                error = qsTr( "Flow shift is not valid." )
            }

            if (error.length !== 0) {
                zImageErrorDialog.text = error
                zImageErrorDialog.open()
                return false
            }

            return true
        }
    }
}