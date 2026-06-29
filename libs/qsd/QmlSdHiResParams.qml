import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {

    property QSdHiResParams hiResParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("HiRes Settings") }
        MenuSeparator{Layout.fillWidth: true  }
        Frame{
            RowLayout{
                // Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
                Switch{
                    text: qsTr("Enable HiRes")
                    checked: hiResParams.isEnabled
                    onClicked: hiResParams.isEnabled = checked
                }
                // Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged FINAL)
                RowLayout {
                    Layout.fillWidth: true
                    Label{ text: qsTr("modelPath:") }
                    TextField{
                        enabled: hiResParams.isEnabled
                        text: hiResParams.modelPath;
                        onAccepted: hiResParams.modelPath = text
                    }
                }
            }
        }
        Frame{
            visible: hiResParams.isEnabled
            GridLayout{

                // Q_PROPERTY(QSdEnums::QSdHiResUpscalerTypes upscaler READ upscaler WRITE setUpscaler NOTIFY upscalerChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Reuse Threshold:")}
                    ComboBox{
                        enabled: hiResParams.isEnabled
                        model: QSdEnums.hiResUpscalerTypesList
                        currentIndex: hiResParams.upscaler
                        onAccepted: hiResParams.upscaler = currentIndex
                    }
                }

                // Q_PROPERTY(float scale READ scale WRITE setScale NOTIFY scaleChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Scale:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0.00
                        value: hiResParams.scale
                        onValueChanged: hiResParams.scale = parseFloat(value)
                    }
                }

                // Q_PROPERTY(int targetWidth READ targetWidth WRITE setTargetWidth NOTIFY targetWidthChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Target Width:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0
                        value: hiResParams.targetWidth
                        onValueChanged: hiResParams.targetWidth = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(int targetHeight READ targetHeight WRITE setTargetHeight NOTIFY targetHeightChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Target Height:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0
                        value: hiResParams.targetHeight
                        onValueChanged: hiResParams.targetHeight = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(int steps READ steps WRITE setSteps NOTIFY stepsChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Steps:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0
                        value: hiResParams.steps
                        onValueChanged: hiResParams.steps = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(float denoisingStrength READ denoisingStrength WRITE setDenoisingStrength NOTIFY denoisingStrengthChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Denoising Strength:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0.00
                        value: hiResParams.denoisingStrength
                        onValueChanged: hiResParams.denoisingStrength = parseFloat(value)
                    }
                }

                // Q_PROPERTY(int upscaleTileSize READ upscaleTileSize WRITE setUpscaleTileSize NOTIFY upscaleTileSizeChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Upscale Tile Size:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0
                        value: hiResParams.upscaleTileSize
                        onValueChanged: hiResParams.upscaleTileSize = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(float* customSigmas READ customSigmas WRITE setCustomSigmas NOTIFY customSigmasChanged FINAL)

                // Q_PROPERTY(int customSigmasCount READ customSigmasCount WRITE setCustomSigmasCount NOTIFY customSigmasCountChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Upscale Tile Size:")}
                    SpinBox{
                        enabled: hiResParams.isEnabled
                        from: 0
                        value: hiResParams.customSigmasCount
                        onValueChanged: hiResParams.customSigmasCount = parseInt(value, 10)
                    }
                }

            }
        }
    }
}