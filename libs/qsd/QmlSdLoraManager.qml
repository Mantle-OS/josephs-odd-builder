import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QSd

Page{
    ColumnLayout{
        Layout.fillWidth: true
        Layout.fillwidth: true
        Button{
            text: qsTr("add")
            onClicked: loraPicker.open()
        }

        ScrollView{
            Layout.fillWidth: true
            Layout.fillwidth: true
            ListView{
                id: loraView
                model: QSd.ImageGenerationParams.loras
                delegate: QmlSdLora {
                    required property int index
                    loraParams: QSd.ImageGenerationParams.loras[index]
                    Layout.fillWidth: true
                    Layout.fillwidth: true
                    onClicked: loraView.currentIndex = index
                }
            }
        }
    }
    FileDialog {
        id: loraPicker

        title: qsTr("Select LoRA")
        fileMode: FileDialog.OpenFile
        nameFilters: [
            qsTr("LoRA files (*.safetensors *.ckpt)"),
            qsTr("All files (*)")
        ]

        property QSdLora newLora: QSdLora{}

        onAccepted: {
            let path = selectedFile.toString()
            if (path.startsWith("file://"))
                path = decodeURIComponent(path.substring(7))

            newLora.path = path
            QSd.ImageGenerationParams.loras.append(newLora)
        }
    }
}


