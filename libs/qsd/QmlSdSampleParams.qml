import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    Layout.fillHeight: true
    Layout.fillWidth: true
    property QSdSampleParams samplerParams: null

    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Sampler Settings") }
        MenuSeparator{Layout.fillWidth: true  }

            Frame{
                GridLayout{
                    columns: 3
                    // Q_PROPERTY(QSdEnums::QSdSchedulerTypes scheduler READ scheduler WRITE setScheduler NOTIFY schedulerChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Scheduler Types:")}
                        ComboBox {
                            model: QSdEnums.schedulerTypesList
                            currentIndex: samplerParams.scheduler //QSD.imgGenParams.sampleParams
                            onAccepted: samplerParams.scheduler = currentIndex
                        }
                    }

                    // Q_PROPERTY(QSdEnums::QSdSampleTypes sampleMethod READ sampleMethod WRITE setSampleMethod NOTIFY sampleMethodChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Sample Method Types:")}
                        ComboBox {
                            model: QSdEnums.sampleTypesList
                            currentIndex: samplerParams.sampleMethod //QSD.imgGenParams.sampleParams
                            onAccepted: samplerParams.sampleMethod = currentIndex
                        }
                    }
                    // Q_PROPERTY(int sampleSteps READ sampleSteps WRITE setSampleSteps NOTIFY sampleStepsChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Sample Steps:")}
                        ComboBox {
                            model: 256
                            currentIndex: samplerParams.sampleSteps //QSD.imgGenParams.sampleParams
                            onAccepted: QSD.imgGenParams.sampleParams.sampleSteps = currentIndex
                        }
                    }
                }
            }

            Frame{
                GridLayout{
                    columns: 4
                    // Q_PROPERTY(float eta READ eta WRITE setEta NOTIFY etaChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("ETA:")}
                        Label{text: samplerParams.eta}
                    }
                    // Q_PROPERTY(int shiftedTimestep READ shiftedTimestep WRITE setShiftedTimestep NOTIFY shiftedTimestepChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Shifted Timestep:")}
                        Label{text: samplerParams.shiftedTimestep}
                    }

                    // Q_PROPERTY(float *customSigmas READ customSigmas WRITE setCustomSigmas NOTIFY customSigmasChanged FINAL)


                    // Q_PROPERTY(int customSigmasCount READ customSigmasCount WRITE setCustomSigmasCount NOTIFY customSigmasCountChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Custom Sigmas Count:")}
                        Label{text: samplerParams.customSigmasCount}
                    }
                    // Q_PROPERTY(float flowShift READ flowShift WRITE setFlowShift NOTIFY flowShiftChanged FINAL)
                    RowLayout{
                        Label{text: qsTr("Flow Shift:")}
                        Label{text: samplerParams.flowShift}
                    }


                    // Q_PROPERTY(QString extraSampleArgs READ extraSampleArgs WRITE setExtraSampleArgs NOTIFY extraSampleArgsChanged FINAL)
                    RowLayout{
                        Label{ text: qsTr("Extra Sample Args:")}
                        TextField{readOnly: true; text: samplerParams.extraSampleArgs; Layout.fillWidth: true }
                    }
                }
            }


            // QP_PTR_RO(QSdGuidanceParams, guidance)
            QmlSdGuidanceParams{
                Layout.fillHeight: true
                Layout.fillWidth: true
                guidanceParams: sampleParams.guidance
            }
    }
}