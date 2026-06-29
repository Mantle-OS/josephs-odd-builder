import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd

Pane{

    property QSdLora loraParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("LoRA Settings") }
        MenuSeparator{Layout.fillWidth: true  }

        // Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged FINAL)
        RowLayout{
            Label{text: qsTr("LoRA Path:")}
            TextField{readOnly: true; text: loraParams.path ; Layout.fillWidth: true }
            Button{
                text: qsTr("Change") ;
                onClicked: {
                    fileManager.open();
                    openFileManager("loraParamsPath")
                }
            }
        }
        // Q_PROPERTY(float multiplier READ multiplier WRITE setMultiplier NOTIFY multiplierChanged FINAL)
        RowLayout{
            Label{text: qsTr("LoRA multiplier:")}
            SpinBox{
                from: 0.00
                value: loraParams.multiplier
                onValueChanged: loraParams.multiplier = parseFloat(value)
            }
        }

        // Q_PROPERTY(bool isHighNoise READ isHighNoise WRITE setIsHighNoise NOTIFY isHighNoiseChanged FINAL)
        Switch{
            text: "Is High Noise"
            checked: loraParams.isHighNoise
            onClicked:  loraParams.isHighNoise = checked
        }

    }
}