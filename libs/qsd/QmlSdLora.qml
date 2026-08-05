import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd

ItemDelegate {
    width: ListView.view
        ? ListView.view.width
        : implicitWidth

    property QSdLora loraParams: null

    RowLayout {
        anchors.fill: parent

        Label {
            text: qsTr("LoRA Path: ") + loraParams.path
            Layout.fillWidth: true
        }

        RowLayout {
            Label {
                text: qsTr("Strength:")
            }

            SpinBox {
                value: loraParams.multiplier
                onValueChanged: loraParams.multiplier = value
            }
        }

        Switch {
            text: qsTr("Is High Noise")
            checked: loraParams.isHighNoise
            onClicked: loraParams.isHighNoise = checked
        }

        Button {
            text: qsTr("X")
            onClicked: QSD.ImageGenerationParams.loras.remove(loraParams)
        }
    }
}