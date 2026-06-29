import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd
Pane {
    property QSdGuidanceParams guidanceParams: null

    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Guidance Settings") }
        MenuSeparator{Layout.fillWidth: true  }
        Frame{
            GridLayout{
                // Q_PROPERTY(float txtCfg READ txtCfg WRITE setTxtCfg NOTIFY txtCfgChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Txt cfg:")}
                    SpinBox{
                        from: 0.0
                        value: guidanceParams.txtCfg
                        onValueChanged: guidanceParams.txtCfg = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(float imgCfg READ imgCfg WRITE setImgCfg NOTIFY imgCfgChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Image cfg:")}
                    SpinBox{
                        from: 0.0
                        value: guidanceParams.imgCfg
                        onValueChanged: guidanceParams.imgCfg = parseInt(value, 10)
                    }
                }
                // Q_PROPERTY(float distilledGuidance READ distilledGuidance WRITE setDistilledGuidance NOTIFY distilledGuidanceChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Distilled guidance:")}
                    SpinBox{
                        from: 0.0
                        value: guidanceParams.imgCfg
                        onValueChanged: guidanceParams.imgCfg = parseInt(value, 10)
                    }
                }
            }
        }
        // QP_PTR_RO(QSdSlgParams, slg)
        QmlSdSlgParams{
            Layout.fillHeight: true
            Layout.fillWidth: true
            slg: guidanceParams.slg
        }
    }
}