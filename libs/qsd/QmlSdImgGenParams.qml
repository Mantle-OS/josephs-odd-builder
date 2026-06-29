import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    property QSdImgGenParams imgGenParams: null

    //     QP_PTR_RW(QSdImage, maskImage)
    //     QP_PTR_RW(QSdImage, initImage);
    //     QP_PTR_RO(QSdImage, refImages);
    //     QP_PTR_RW(QSdImage, controlImage)

    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true


        Label{ text: qsTr("Image Geneneration Settings") }
        MenuSeparator{Layout.fillWidth: true  }

        Frame{
            Layout.fillHeight: true
            Layout.fillWidth: true
            //     Q_PROPERTY(QString prompt READ prompt WRITE setPrompt NOTIFY promptChanged FINAL)
            ColumnLayout{
                RowLayout {
                    Label{
                        text: qsTr("Prompt:")
                    }
                    TextField{
                        text: imgGenParams.prompt ;
                        onAccepted: imgGenParams.prompt = text
                        Layout.fillWidth: true
                    }
                }

                //     Q_PROPERTY(QString negativePrompt READ negativePrompt WRITE setNegativePrompt NOTIFY negativePromptChanged FINAL)
                RowLayout {
                    Label{ text: qsTr("Negative Prompt:") }
                    TextField{
                        Layout.fillWidth: true
                        text: imgGenParams.negativePrompt
                        onAccepted: imgGenParams.negativePrompt = text
                    }
                }
            }
        }

        Frame{
            Layout.fillHeight: true
            Layout.fillWidth: true
            GridLayout{
                //     Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Width:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.width
                        onValueChanged: imgGenParams.width = parseInt(value, 10)
                    }
                }

                //     Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Height:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.height
                        onValueChanged: imgGenParams.height = parseInt(value, 10)
                    }
                }


                //     Q_PROPERTY(qint64 seed READ seed WRITE setSeed NOTIFY seedChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Seed:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.seed
                        // to: 9223372036854775807n
                        onValueChanged: imgGenParams.seed = parseInt(value, 10)
                    }
                }

                //     Q_PROPERTY(int batchCount READ batchCount WRITE setBatchCount NOTIFY batchCountChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Batch Count:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.batchCount
                        onValueChanged: imgGenParams.batchCount = parseInt(value, 10)
                    }
                }

                //     Q_PROPERTY(float strength READ strength WRITE setStrength NOTIFY strengthChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Strength:")}
                    SpinBox{
                        from: 0.00
                        value: imgGenParams.strength
                        to: 1
                        onValueChanged: imgGenParams.strength = parseFloat(value)
                    }
                }

                //     Q_PROPERTY(int clipSkip READ clipSkip WRITE setClipSkip NOTIFY clipSkipChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Clip Skip:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.clipSkip
                        onValueChanged: imgGenParams.clipSkip = parseInt(value, 10)
                    }
                }

                //     Q_PROPERTY(int refImagesCount READ refImagesCount WRITE setRefImagesCount NOTIFY refImagesCountChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Refernce Images Count:")}
                    SpinBox{
                        from: 0
                        value: imgGenParams.refImagesCount
                        onValueChanged: imgGenParams.refImagesCount = parseInt(value, 10)
                    }
                }

                //     Q_PROPERTY(bool increaseRefIndex READ increaseRefIndex WRITE setIncreaseRefIndex NOTIFY increaseRefIndexChanged FINAL)
                Switch{
                    text: qsTr("Increase Ref Index ")
                    checked:   imgGenParams.increaseRefIndex
                    onClicked: imgGenParams.increaseRefIndex = checked
                }

                //     Q_PROPERTY(float controlStrength READ controlStrength WRITE setControlStrength NOTIFY controlStrengthChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Control Strength:")}
                    SpinBox{
                        from: 0.00
                        value: imgGenParams.controlStrength
                        to: 9
                        onValueChanged: imgGenParams.controlStrength = parseFloat(value)
                    }
                }
            }
        }


        Frame{
            ColumnLayout{


                //     QP_PTR_RW(QSdSampleParams, sampleParams)
                QmlSdSampleParams{
                    samplerParams: imgGenParams.sampleParams
                }
                //     QP_PTR_RO(QSdCacheParams, cache)
                QmlSdCacheParams{
                    cacheParams: imgGenParams.cache
                }


                //     QP_PTR_RO(QSdTilingParams, vaeTilingParams)
                QmlSdTilingParams{
                    tilingParams: imgGenParams.vaeTilingParams
                }

                QmlSdPmParams{
                    pmParams: imgGenParams.pmParams
                }

                //     QP_PTR_RO(QSdHiResParams, hires)
                QmlSdHiResParams{
                    hiResParams: imgGenParams.hires
                }
                //     QP_PTR_RO(QSdLora, loras) // LOOK INTO MODEL ?
                QmlSdLora {
                    loraParams: imgGenParams.loras
                }

                //     Q_PROPERTY(quint32 loraCount READ loraCount WRITE setLoraCount NOTIFY loraCountChanged FINAL)
                // RowLayout{
                //     Label{text: qsTr("Lora Count:")}
                //     SpinBox{
                //         enabled: false
                //         from: 0
                //         value: imgGenParams.loraCount
                //         // onValueChanged: QSD.imgGenParams.clipSkip = parseInt(value, 10)
                //     }
                // }
                //     QP_PTR_RO(QSdPmParams, pmParams)
            }
        }



    }
}
