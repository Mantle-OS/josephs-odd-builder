import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane{
    property QSdCtxParams ctxParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Paths and Files") }
        MenuSeparator{Layout.fillWidth: true  }


        Frame{
            Layout.fillHeight: true
            Layout.fillWidth: true
            GridLayout{
                columns: 3
                // Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Model Path: ")}
                    TextField{readOnly: true; text: QSD.ContextParams.modelPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("modelPath")
                        }
                    }
                }
                // Q_PROPERTY(QString clipLPath READ getClipLPath WRITE setClipLPath NOTIFY clipLPathChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Clip L Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.clipLPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("modelPath")
                        }
                    }
                }

                // Q_PROPERTY(QString clipGPath READ getClipGPath WRITE setClipGPath NOTIFY clipGPathChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Clip G Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.clipGPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("clipGPath")
                        }
                    }
                }
                // Q_PROPERTY(QString clipVisionPath READ clipVisionPath WRITE setClipVisionPath NOTIFY clipVisionPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("clip Vision Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.clipGPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("clipVisionPath")
                        }
                    }
                }
                // Q_PROPERTY(QString t5xxlPath READ t5xxlPath WRITE setT5xxlPath NOTIFY t5xxlPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("t5xxl Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.t5xxlPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("clipVisionPath")
                        }
                    }
                }
                // Q_PROPERTY(QString llmPath READ llmPath WRITE setLlmPath NOTIFY llmPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("LLM Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.llmPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("llmPath")
                        }
                    }
                }

                // Q_PROPERTY(QString llmVisionPath READ llmVisionPath WRITE setLlmVisionPath NOTIFY llmVisionPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("LLM Vision Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.llmVisionPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("llmVisionPath")
                        }
                    }
                }

                // Q_PROPERTY(QString diffusionModelPath READ diffusionModelPath WRITE setDiffusionModelPath NOTIFY diffusionModelPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Diffusion Model Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.diffusionModelPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("diffusionModelPath")
                        }
                    }
                }

                // Q_PROPERTY(QString highNoiseDiffusionModelPath READ highNoiseDiffusionModelPath WRITE setHighNoiseDiffusionModelPath NOTIFY highNoiseDiffusionModelPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("High Noise Diffusion Model Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.diffusionModelPath ; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("diffusionModelPath")
                        }
                    }
                }

                // Q_PROPERTY(QString uncondDiffusionModelPath READ uncondDiffusionModelPath WRITE setUncondDiffusionModelPath NOTIFY uncondDiffusionModelPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Uncond Diffusion Model Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.uncondDiffusionModelPath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("uncondDiffusionModelPath")
                        }
                    }
                }
                // Q_PROPERTY(QString embeddingsConnectorsPath READ embeddingsConnectorsPath WRITE setEmbeddingsConnectorsPath NOTIFY embeddingsConnectorsPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Embeddings Connectors Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.embeddingsConnectorsPath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("embeddingsConnectorsPath")
                        }
                    }
                }

                // Q_PROPERTY(QString vaePath READ vaePath WRITE setVaePath NOTIFY vaePathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("VAE Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.vaePath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("vaePath")
                        }
                    }
                }
                // Q_PROPERTY(QString audioVaePath READ audioVaePath WRITE setAudioVaePath NOTIFY audioVaePathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Audio Vae Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.audioVaePath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("audioVaePath")
                        }
                    }
                }

                // Q_PROPERTY(QString taesdPath READ taesdPath WRITE setTaesdPath NOTIFY taesdPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Taesd Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.taesdPath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("taesdPath")
                        }
                    }
                }
                // Q_PROPERTY(QString controlNetPath READ controlNetPath WRITE setControlNetPath NOTIFY controlNetPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("ControlNet Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.controlNetPath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("controlNetPath")
                        }
                    }
                }

                // Q_PROPERTY(QString photoMakerPath READ photoMakerPath WRITE setPhotoMakerPath NOTIFY photoMakerPathChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Photo Maker Path:")}
                    TextField{readOnly: true; text: QSD.ContextParams.photoMakerPath; Layout.fillWidth: true }
                    Button{
                        text: qsTr("Change") ;
                        onClicked: {
                            fileManager.open();
                            openFileManager("photoMakerPath")
                        }
                    }
                }
            }
        }

        Frame{
            Layout.fillHeight: true
            Layout.fillWidth: true
            GridLayout{
                columns: 4
                // Q_PROPERTY(QString tensorTypeRules READ tensorTypeRules WRITE setTensorTypeRules NOTIFY tensorTypeRulesChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Tensor Type Rules:")}
                    TextField{readOnly: true; text: ctxParams.tensorTypeRules; Layout.fillWidth: true }
                }

                // Q_PROPERTY(QString backend READ backend WRITE setBackend NOTIFY backendChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Backend:")}
                    TextField{readOnly: true; text: ctxParams.backend; Layout.fillWidth: true }
                }
                // Q_PROPERTY(QString paramsBackend READ paramsBackend WRITE setParamsBackend NOTIFY paramsBackendChanged FINAL)
                RowLayout{
                    Label{ text: qsTr("Params Backend:")}
                    TextField{readOnly: true; text: ctxParams.paramsBackend; Layout.fillWidth: true }
                }

                RowLayout {
                    Label{ text: qsTr("Rpc Servers:") }
                    TextField{ text: ctxParams.rpcServers ; onAccepted: ctxParams.rpcServers = text}
                }
            }
        }
        // ENUMS
        Label{ text: qsTr("Enums") }
        MenuSeparator{Layout.fillWidth: true}
        Frame{
            Layout.fillHeight: true
            Layout.fillWidth: true
            GridLayout{
                columns: 4
                //     Q_PROPERTY(QSdEnums::QSdWeightTypes wType READ getWType WRITE setWType NOTIFY wTypeChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Weight Type:")}
                    ComboBox {
                        model: QSdEnums.weightTypesList
                        currentIndex: ctxParams.wType
                        onAccepted: ctxParams.wType = parseInt(currentIndex, 10)
                    }
                }

                //     Q_PROPERTY(QSdEnums::QSdRngTypes rngType READ rngType WRITE setRngType NOTIFY rngTypeChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Rng Type:")}
                    ComboBox {
                        model: QSdEnums.rngTypesList
                        currentIndex: ctxParams.rngType
                        onAccepted: ctxParams.rngType = parseInt(currentIndex)
                    }
                }

                //     Q_PROPERTY(QSdEnums::QSdRngTypes samplerRngType READ samplerRngType WRITE setSamplerRngType NOTIFY samplerRngTypeChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Sampler Rng Type:")}
                    ComboBox {
                        model: QSdEnums.rngTypesList
                        currentIndex: ctxParams.rngType
                        onAccepted: ctxParams.rngType = parseInt(currentIndex)
                    }
                }

                //     Q_PROPERTY(QSdEnums::QSdPredictionTypes prediction READ prediction WRITE setPrediction NOTIFY predictionChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Prediction Type:")}
                    ComboBox {
                        model: QSdEnums.predictionTypesList
                        currentIndex: ctxParams.prediction
                        onAccepted: ctxParams.prediction = parseInt(currentIndex)
                    }
                }
                //     Q_PROPERTY(QSdEnums::QSdLoraApplyModeTypes loraApplyMode READ loraApplyMode WRITE setLoraApplyMode NOTIFY loraApplyModeChanged FINAL)
                RowLayout{
                    Label{text: qsTr("LoRA Apply Mode Type:")}
                    ComboBox {
                        model: QSdEnums.loraApplyModeTypesList
                        currentIndex: ctxParams.loraApplyMode
                        onAccepted: ctxParams.loraApplyMode = parseInt(currentIndex)
                    }
                }
                //     Q_PROPERTY(QSdEnums::QSdVaeFormatTypes vaeFormat READ vaeFormat WRITE setVaeFormat NOTIFY vaeFormatChanged FINAL)
                RowLayout{
                    Label{text: qsTr("VAE Format Type:")}
                    ComboBox {
                        model: QSdEnums.vaeFormatTypesList
                        currentIndex: ctxParams.vaeFormat
                        onAccepted: ctxParams.vaeFormat = parseInt(currentIndex)
                    }
                }
            }
        }
        //     // BOOL

        //     QP_PTR_RO(QSdEmbedding, embeddings)
        QmlSdEmbedding{
            embeddingParams: ctxParams.embeddings

        }
        Frame{
            GridLayout{
                columns: 4
                //     Q_PROPERTY(quint32 embeddingCount READ embeddingCount WRITE setEmbeddingCount NOTIFY embeddingCountChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Embedding Count:")}
                    SpinBox{
                        value: ctxParams.embeddingCount
                        onValueChanged:  ctxParams.embeddingCount = value
                    }
                }
                //     Q_PROPERTY(int numberOfThreads READ numberOfThreads WRITE setNumberOfThreads NOTIFY numberOfThreadsChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Number Of Threads:")}
                    SpinBox{
                        from: 0
                        value:  ctxParams.numberOfThreads
                        to: QSD.numPhysicalCores
                        onValueChanged: ctxParams.numberOfThreads = parseInt(value, 10)
                    }
                }
                //     Q_PROPERTY(int chromaT5MaskPad READ chromaT5MaskPad WRITE setChromaT5MaskPad NOTIFY chromaT5MaskPadChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Chroma T5 Mask Pad:")}
                    SpinBox{
                        value:  ctxParams.chromaT5MaskPad
                        from: 0
                        to: 256
                        onValueChanged: ctxParams.embeddingCount = value
                    }
                }

                //     Q_PROPERTY(float maxVram READ maxVram WRITE setMaxVram NOTIFY maxVramChanged FINAL)
                //     // GiB budget for graph-cut segmented param offload (0 = disabled, -1 = auto free VRAM minus 1 GiB)
                RowLayout{
                    Label{text: qsTr("Max Vram:")}
                    SpinBox{
                        value:  ctxParams.maxVram
                        from: -1
                        to: 16
                        onValueChanged: ctxParams.embeddingCount = value
                    }
                }
            }
        }

        // START BOOL
        Frame {
            GridLayout{
                columns: 8
                Switch{
                    text: qsTr("Enable MMap")
                    checked: ctxParams.enableMmap
                    onClicked: ctxParams.enableMmap = checked
                }

                Switch{
                    text: qsTr("Flash Attn")
                    checked: ctxParams.flashAttn
                    onClicked: ctxParams.flashAttn = checked
                }

                Switch{
                    text: qsTr("Diffusion Flash Attn")
                    checked: ctxParams.diffusionFlashAttn
                    onClicked: ctxParams.diffusionFlashAttn = checked
                }

                Switch{
                    text: qsTr("TAE Preview Only")
                    checked: ctxParams.taePreviewOnly
                    onClicked: ctxParams.taePreviewOnly = checked
                }

                Switch{
                    text: qsTr("Diffusion Conv Direct ")
                    checked: ctxParams.diffusionConvDirect
                    onClicked: ctxParams.diffusionConvDirect  = checked
                }

                Switch{
                    text: qsTr("VAE Conv Direct")
                    checked: ctxParams.vaeConvDirect
                    onClicked: ctxParams.vaeConvDirect = checked
                }

                Switch{
                    text: qsTr("Circular X")
                    checked: ctxParams.circularX
                    onClicked: ctxParams.circularX = checked
                }

                Switch{
                    text: qsTr("Circular Y")
                    checked: ctxParams.circularY
                    onClicked: ctxParams.circularY = checked
                }

                Switch{
                    text: qsTr("Force SDXL VAE Conv Scale")
                    checked: ctxParams.forceSdxlVaeConvScale
                    onClicked: ctxParams.forceSdxlVaeConvScale = checked
                }
                Switch{
                    text: qsTr("Chroma Use Dit Mask")
                    checked: ctxParams.chromaUseDitMask
                    onClicked: ctxParams.chromaUseDitMask = checked
                }
                Switch{
                    text: qsTr("Chroma Use T5 Mask")
                    checked: ctxParams.chromaUseT5Mask
                    onClicked: ctxParams.chromaUseT5Mask = checked
                }
                Switch{
                    text: qsTr("Stream Layers")
                    checked: ctxParams.streamLayers
                    onClicked: ctxParams.streamLayers = checked
                }
                Switch{
                    text: qsTr("Qwen Zero Cond")
                    checked: ctxParams.qwenImageZeroCondT
                    onClicked: ctxParams.qwenImageZeroCondT = checked
                }

            }
        }
    }
}
