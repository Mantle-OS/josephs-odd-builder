import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Window
import QSd

ApplicationWindow {
    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    title: qsTr("ZImage Turbo test")

    //
    //DONE   --diffusion-model  z_image_turbo-Q3_K.gguf
    //DONE  --vae ..\..\ComfyUI\models\vae\ae.sft
    //DONE  --llm ..\..\ComfyUI\models\text_encoders\Qwen3-4B-Instruct-2507-Q4_K_M.gguf
    //DONE  -p ""

    //DONE --cfg-scale 1.0

    // -v

    // --offload-to-cpu
    // "--offload-to-cpu",
    //      "place the weights in RAM to save VRAM, and automatically load them into VRAM when needed",
    //      true, &offload_params_to_cpu},


    //DONE --diffusion-fa

    //DONE  -H 1024
    //DONE  -W 512

    //DONE --steps 8

    // Label {text: QSD.systemInfo }
    // Label {text: "Cores: " + QSD.numPhysicalCores }
    // Label {text: "Stable diff cpp version: " + QSD.sdVersion + "_" + QSD.sdCommit}
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action { text: qsTr("&New...") }
            Action { text: qsTr("&Open...") }
            Action { text: qsTr("&Save") }
            Action { text: qsTr("Save &As...") }
            MenuSeparator { }
            Action { text: qsTr("&Quit") }
        }
        Menu {
            title: qsTr("&Edit")
            Action { text: qsTr("Cu&t") }
            Action { text: qsTr("&Copy") }
            Action { text: qsTr("&Paste") }
        }
    }
    header: TabBar{
        id: bar
        width: parent.width

        TabButton{
            text: qsTr("Context")
        }
        TabButton{
            text: qsTr("Image Generation")
        }
        TabButton{
            text: qsTr("Devices")
        }
        TabButton{
            text: qsTr("ZImage Turbo")
        }
        TabButton{
            text: qsTr("SD3 Large")
        }
        TabButton{
            text: qsTr("SD3 Medium")
        }
    }

    StackLayout{
        id: stackView
        currentIndex: bar.currentIndex
        anchors.fill: parent
        QmlSdCtxParams {
            ctxParams:  QSD.ctxParams
        }

        QmlSdImgGenParams{
            imgGenParams: QSD.imgGenParams
        }

        QmlSdBackendManager{

        }
        ZImageTurbo{

        }
        SDThree{
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        SDThreeMedium{

        }

    }
    RowLayout{
        ProgressBar{
            from: 0.0
            to: QSD.totalSteps
            value: QSD.currentStep
        }
        Label{ text: QSD.currentStep + "/" + QSD.totalSteps + " " + QSD.progressionTime}
    }


    // footer: ScrollView{
    //     Layout.fillWidth: true
    //     Layout.fillHeight: false
    //     Layout.preferredHeight: parent.height / 4
    //     Layout.preferredWidth: parent.width
    //     TextArea{
    //         id: consoleText
    //         visible: QSD.logToConsole
    //         Connections {
    //             target: QSD
    //             function onLastLogChanged() {
    //                 consoleText.append(QSD.lastLog)
    //             }
    //         }
    //     }
    // }










    ////////////// POPUPS

    FileDialog{ id: fileManager }




    // Button{
    //     text: qsTr("Generate Image")
    //     onClicked: {
    //         // models
    //         QSD.ctxParams.modelPath             = "/srv/ai/ComfyUI/models/unet/ZImage_Turbo/z-image-turbo-Q4_K_M.gguf"
    //         QSD.ctxParams.diffusionModelPath    = "/srv/ai/ComfyUI/models/unet/ZImage_Turbo/z-image-turbo-Q4_K_M.gguf"
    //         QSD.ctxParams.llmPath               = "/srv/ai/ComfyUi/models/vae/qwen-4b-zimage-hereticV2-q8.gguf"
    //         QSD.ctxParams.vaePath               = "/srv/ai/ComfyUI/models/vae/ZImage_Turbo/whatever.safetensors"

    //         // base
    //         QSD.imgGenParams.width                          = 512
    //         QSD.imgGenParams.height                         = 1028
    //         QSD.imgGenParams.prompt                         = pos_text.text
    //         QSD.imgGenParams.seed                           = 42
    //         // --steps 8
    //         QSD.imgGenParams.sampleParams.sampleSteps       = 8
    //         // guidence (--cfg-scale = 1.0)
    //         QSD.imgGenParams.sampleParams.guidance.txtCfg   = parseFloat(1.0)
    //         // --diffusion-fa
    //         QSD.ctxParams.diffusionFlashAttn                = true

    //         // dgbctxParamsTxt.clear()
    //         // dgbctxParamsTxt.append(QSD.ctxParams.debugString())

    //         // imgGenParamsTxt.clear()
    //         // imgGenParamsTxt.append(QSD.imgGenParams.debugString())

    //         if( QSD.supportsImageGen() ){
    //             console.log("Generating Image")
    //             QSD.generateImage()
    //         }else{
    //             console.log("Generating Image Not supported ?")
    //         }
    //     }
    // }
    // }






}


