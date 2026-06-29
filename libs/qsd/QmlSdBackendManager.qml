import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd
Pane{
    ColumnLayout {
        Label {
            text: "Fine-Grained Hardware Layer Routing (" + QSD.Backend.count + ")"
            font.bold: true
        }

        ListView{
            model: QSD.Backend
            Layout.fillHeight: true
            Layout.fillWidth: true
            delegate: Pane{

                RowLayout {
                    spacing: 15

                    Label {
                        text: "Module: " + model.qtObject.moduleType // Translated via Enum maps
                        Layout.preferredWidth: 150
                    }

                    ComboBox {
                        model: ["cuda", "cpu", "vulkan", "rpc"]
                        currentIndex: model.runtimeTarget === "cuda" ? 0 : (model.runtimeTarget === "cpu" ? 1 : 2)
                        onActivated: model.runtimeTarget = currentText
                    }

                    ComboBox {
                        model: ["cuda", "cpu", "mmap_disk"]
                        currentIndex: model.paramsTarget === "cuda" ? 0 : 1
                        onActivated: model.paramsTarget = currentText
                    }

                    SpinBox {
                        visible: model.runtimeTarget === "cpu"
                        from: 1; to: 32
                        value: model.cpuThreadLimit
                        onValueChanged: model.cpuThreadLimit = value
                    }
                }
            }
        }
    }
}
