import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    property QSdSlgParams slg: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Slg Settings ") +  slg.layerCount }
        MenuSeparator{Layout.fillWidth: true  }

        // Q_PROPERTY(int *layers READ layers WRITE setLayers NOTIFY layersChanged FINAL)
        // Q_PROPERTY(qsizetype layerCount READ layerCount WRITE setLayerCount NOTIFY layerCountChanged FINAL)
        // Label{text: qsTr("Layer Size: " + slg.layerCount)}

        Frame{
            RowLayout{
                // Q_PROPERTY(float layerStart READ layerStart WRITE setLayerStart NOTIFY layerStartChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Layer Start:")}
                    SpinBox {
                        from: 0.0
                        value: slg.layerStart
                        onValueChanged: slg.layerStart = parseInt(value, 10)
                    }
                }


                // Q_PROPERTY(float layerEnd READ layerEnd WRITE setLayerEnd NOTIFY layerEndChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Layer End:")}
                    SpinBox {
                        from: 0.0
                        value: slg.layerEnd
                        onValueChanged: slg.layerEnd = parseInt(value, 10)
                    }
                }

                // Q_PROPERTY(float scale READ scale WRITE setScale NOTIFY scaleChanged FINAL)
                RowLayout{
                    Label{text: qsTr("Scale:")}
                    SpinBox {
                        from: 0.0
                        value: slg.scale
                        to: 1.0
                        onValueChanged: slg.scale = parseInt(value, 10)
                    }
                }
            }
        }
    }
}