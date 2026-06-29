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
                    QSD.ContextParams.modelPath                             = "/srv/ai/ComfyUI/models/checkpoints/SD3/sd3.5_large.safetensors"
                    QSD.ContextParams.clipLPath                             = "/srv/ai/ComfyUI/models/checkpoints/SD3/clip_l.safetensors"
                    QSD.ContextParams.clipGPath                             = "/srv/ai/ComfyUI/models/checkpoints/SD3/clip_g.safetensors"
                    QSD.ContextParams.t5xxlPath                             = "/srv/ai/ComfyUI/models/checkpoints/SD3/t5xxl_fp16.safetensors"
                    QSD.ContextParams.diffusionFlashAttn                    = true
                    QSD.ContextParams.weightsOnCpu                          = true

                    // GenerationParams
                    QSD.ImageGenerationParams.width                          = 512
                    QSD.ImageGenerationParams.height                         = 1024
                    QSD.ImageGenerationParams.seed                           = 42
                    QSD.ImageGenerationParams.prompt                         = "a lovely cat holding a sign says \"Qt Stable diffusion 3.5 Large\""
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