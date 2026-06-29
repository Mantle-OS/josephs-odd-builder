import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    property QSdCacheParams cacheParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Cache Settings") }
        MenuSeparator{Layout.fillWidth: true  }

        GridLayout{
            columns: 5
            //     Q_PROPERTY(QSdEnums::QSdCacheModeTypes mode READ mode WRITE setMode NOTIFY modeChanged FINAL)
            RowLayout{
                Label{text: qsTr("Reuse Threshold:")}
                ComboBox{
                    model: QSdEnums.cacheModeTypesList
                    currentIndex: cacheParams.mode
                    onAccepted: cacheParams.mode = currentIndex
                }
            }

            //     Q_PROPERTY(float reuseThreshold READ reuseThreshold WRITE setReuseThreshold NOTIFY reuseThresholdChanged FINAL)
            RowLayout{
                Label{text: qsTr("Reuse Threshold:")}
                SpinBox{
                    from: 0.0
                    value: cacheParams.reuseThreshold
                    onValueChanged: cacheParams.reuseThreshold = parseFloat(value)
                }
            }

            //     Q_PROPERTY(float startPercent READ startPercent WRITE setStartPercent NOTIFY startPercentChanged FINAL)
            RowLayout{
                Label{text: qsTr("Start Percent:")}
                SpinBox{
                    from: 0.0
                    value: cacheParams.startPercent
                    onValueChanged: cacheParams.startPercent = parseFloat(value)
                }
            }

            //     Q_PROPERTY(float endPercent READ endPercent WRITE setEndPercent NOTIFY endPercentChanged FINAL)
            RowLayout{
                Label{text: qsTr("End Percent:")}
                SpinBox{
                    from: 0.0
                    value: cacheParams.endPercent
                    onValueChanged: cacheParams.endPercent = parseFloat(value)
                }
            }

            //     Q_PROPERTY(float errorDecayRate READ errorDecayRate WRITE setErrorDecayRate NOTIFY errorDecayRateChanged FINAL)
            RowLayout{
                Label{text: qsTr("Error Decay Rate:")}
                SpinBox{
                    from: 0.0
                    value: cacheParams.errorDecayRate
                    onValueChanged: cacheParams.errorDecayRate = parseFloat(value)
                }
            }

            //     Q_PROPERTY(bool useRelativeThreshold READ useRelativeThreshold WRITE setUseRelativeThreshold NOTIFY useRelativeThresholdChanged FINAL)
            Switch{
                text: qsTr("Use Relative Threshold")
                checked: cacheParams.useRelativeThreshold
                onClicked: cacheParams.useRelativeThreshold = checked
            }

            //     Q_PROPERTY(bool resetErrorOnCompute READ resetErrorOnCompute WRITE setResetErrorOnCompute NOTIFY resetErrorOnComputeChanged FINAL)
            Switch{
                text: qsTr("Reset Error On Compute")
                checked: cacheParams.resetErrorOnCompute
                onClicked: cacheParams.resetErrorOnCompute = checked
            }

            //     Q_PROPERTY(int fnComputeBlocks READ fnComputeBlocks WRITE setFnComputeBlocks NOTIFY fnComputeBlocksChanged FINAL)
            RowLayout{
                Label{text: qsTr("Fn Compute Blocks:")}
                SpinBox{
                    from: 0
                    value: cacheParams.fnComputeBlocks
                    onValueChanged: cacheParams.fnComputeBlocks = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(int bnComputeBlocks READ bnComputeBlocks WRITE setBnComputeBlocks NOTIFY bnComputeBlocksChanged FINAL)
            RowLayout{
                Label{text: qsTr("Bn Compute Blocks:")}
                SpinBox{
                    from: 0
                    value: cacheParams.bnComputeBlocks
                    onValueChanged: cacheParams.bnComputeBlocks = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(float residualDiffThreshold READ residualDiffThreshold WRITE setResidualDiffThreshold NOTIFY residualDiffThresholdChanged FINAL)
            RowLayout{
                Label{text: qsTr("Residual Diff Threshold:")}
                SpinBox{
                    from: 0.0
                    value: cacheParams.residualDiffThreshold
                    onValueChanged: cacheParams.residualDiffThreshold = parseFloat(value)
                }
            }

            //     Q_PROPERTY(int maxWarmupSteps READ maxWarmupSteps WRITE setMaxWarmupSteps NOTIFY maxWarmupStepsChanged FINAL)
            RowLayout{
                Label{text: qsTr("Max Warmup Steps:")}
                SpinBox{
                    from: 0
                    value: cacheParams.maxWarmupSteps
                    onValueChanged: cacheParams.maxWarmupSteps = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(int maxCachedSteps READ maxCachedSteps WRITE setMaxCachedSteps NOTIFY maxCachedStepsChanged FINAL)
            RowLayout{
                Label{text: qsTr("Max Cached Steps:")}
                SpinBox{
                    from: 0
                    value: cacheParams.maxCachedSteps
                    onValueChanged: cacheParams.maxCachedSteps = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(int maxContinuousCachedSteps READ maxContinuousCachedSteps WRITE setMaxContinuousCachedSteps NOTIFY maxContinuousCachedStepsChanged FINAL)
            RowLayout{
                Label{text: qsTr("Max Continuous Cached Steps:")}
                SpinBox{
                    from: 0
                    value: cacheParams.maxContinuousCachedSteps
                    onValueChanged: cacheParams.maxContinuousCachedSteps = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(int taylorseerNDerivatives READ taylorseerNDerivatives WRITE setTaylorseerNDerivatives NOTIFY taylorseerNDerivativesChanged FINAL)
            RowLayout{
                Label{text: qsTr("Taylorseer N Derivatives:")}
                SpinBox{
                    from: 0
                    value: cacheParams.taylorseerNDerivatives
                    onValueChanged: cacheParams.taylorseerNDerivatives = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(int taylorseerSkipInterval READ taylorseerSkipInterval WRITE setTaylorseerSkipInterval NOTIFY taylorseerSkipIntervalChanged FINAL)
            RowLayout{
                Label{text: qsTr("Taylorseer Skip Interval:")}
                SpinBox{
                    from: 0
                    value: cacheParams.taylorseerSkipInterval
                    onValueChanged: cacheParams.taylorseerSkipInterval = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(QString scmMask READ scmMask WRITE setScmMask NOTIFY scmMaskChanged FINAL)
            RowLayout {
                Label{ text: qsTr("Scm Mask:") }
                TextField{ text: cacheParams.scmMask; onAccepted: cacheParams.scmMask = text}
            }

            //     Q_PROPERTY(bool scmPolicyDynamic READ scmPolicyDynamic WRITE setScmPolicyDynamic NOTIFY scmPolicyDynamicChanged FINAL)
            Switch{
                text: qsTr("Scm Policy Dynamic")
                checked: cacheParams.scmPolicyDynamic
                onClicked: cacheParams.scmPolicyDynamic = checked
            }

            //     Q_PROPERTY(float spectrumW READ spectrumW WRITE setSpectrumW NOTIFY spectrumWChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum W:")}
                SpinBox{
                    from: 0.00
                    value: cacheParams.spectrumW
                    onValueChanged: cacheParams.spectrumW = parseFloat(value)
                }
            }

            //     Q_PROPERTY(int spectrumM READ spectrumM WRITE setSpectrumM NOTIFY spectrumMChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum M:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumM
                    onValueChanged: cacheParams.spectrumM = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(float spectrumLam READ spectrumLam WRITE setSpectrumLam NOTIFY spectrumLamChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum Lam:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumLam
                    onValueChanged: cacheParams.spectrumLam = parseFloat(value)
                }
            }

            //     Q_PROPERTY(int spectrumWindowSize READ spectrumWindowSize WRITE setSpectrumWindowSize NOTIFY spectrumWindowSizeChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum Window Size:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumWindowSize
                    onValueChanged: cacheParams.spectrumWindowSize = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(float  spectrumFlexWindow READ  spectrumFlexWindow WRITE setSpectrumFlexWindow NOTIFY  spectrumFlexWindowChanged FINAL)
            RowLayout{
                Label{text: qsTr("spectrumFlexWindow:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumFlexWindow
                    onValueChanged: cacheParams.spectrumFlexWindow = parseFloat(value)
                }
            }

            //     Q_PROPERTY(int spectrumWarmupSteps READ spectrumWarmupSteps WRITE setSpectrumWarmupSteps NOTIFY spectrumWarmupStepsChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum Warmup Steps:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumWarmupSteps
                    onValueChanged: cacheParams.spectrumWarmupSteps = parseInt(value, 10)
                }
            }

            //     Q_PROPERTY(float spectrumStopPercent READ spectrumStopPercent WRITE setSpectrumStopPercent NOTIFY spectrumStopPercentChanged FINAL)
            RowLayout{
                Label{text: qsTr("Spectrum Flex Window:")}
                SpinBox{
                    from: 0
                    value: cacheParams.spectrumStopPercent
                    onValueChanged: cacheParams.spectrumStopPercent = parseFloat(value)
                }
            }

        }
    }
}