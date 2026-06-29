import QtQuick

import QtQuick.Controls
import QtQuick.Layouts
import QSd

Pane{
    ColumnLayout{
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignCenter
        Label{text:qsTr("Prompt")}
        MenuSeparator{Layout.fillWidth: true}

        RowLayout{
            Switch{
                id: autoSaveSwitch
                text: qsTr("save file")
                checked:  false
            }
            Button{
                text: qsTr("Generate Image")
                onClicked: {
                    //ContextParams
                    QSD.ContextParams.modelPath                             = "/srv/ai/ComfyUI/models/checkpoints/SD3/sd3_medium_incl_clips_t5xxlfp16.safetensors"
                    QSD.ContextParams.diffusionFlashAttn                    = true
                    QSD.ContextParams.clipOnCpu                             = true


                    // GenerationParams
                    QSD.ImageGenerationParams.width                          = 1024
                    QSD.ImageGenerationParams.height                         = 1024
                    QSD.ImageGenerationParams.seed                           = 42
                    QSD.ImageGenerationParams.prompt                         = "a lovely cat holding a sign says \"Qt Stable diffusion 3 Medium\""
                    QSD.ImageGenerationParams.sampleParams.sampleMethod      = QSdEnums.QSdEuler
                    QSD.ImageGenerationParams.sampleParams.guidance.txtCfg   = parseFloat(4.5)

                    QSD.generateImage(outputImage, autoSaveSwitch.checked)
                }
            }
        }
        /// where we are going to add our cusom time deal
        QSdImage{
            id: outputImage
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
