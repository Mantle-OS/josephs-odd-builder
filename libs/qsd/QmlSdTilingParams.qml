import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

Pane {
    property QSdTilingParams tilingParams: null

    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Tiling Settings") }
        MenuSeparator{Layout.fillWidth: true  }

        GridLayout{
            columns: 6

        // Q_PROPERTY(bool isEnabled READ isEnabled WRITE setIsEnabled NOTIFY isEnabledChanged FINAL)
        Switch{
            id: tileEnabledSwitch
            text: qsTr("Is Enabled")
            checked:  tilingParams.isEnabled
            onClicked: tilingParams.isEnabled = checked
        }

        // Q_PROPERTY(bool temporalTiling READ temporalTiling WRITE setTemporalTiling NOTIFY temporalTilingChanged FINAL)
        Switch{
            enabled: tilingParams.isEnabled
            text: qsTr("Temporal Tiling")
            checked:  tilingParams.temporalTiling
            onClicked: tilingParams.temporalTiling = checked
        }

        // Q_PROPERTY(int tileSizeX READ tileSizeX WRITE setTileSizeX NOTIFY tileSizeXChanged FINAL)
        RowLayout{
            Label{text: qsTr("Tile Size X:")}
            SpinBox {
                from: 0
                value: tilingParams.tileSizeX
                onValueChanged: tilingParams.tileSizeX = parseInt(value, 10)
            }
        }


        // Q_PROPERTY(int tileSizeY READ tileSizeY WRITE setTileSizeY NOTIFY tileSizeYChanged FINAL)
        RowLayout{
            Label{text: qsTr("Tile Size Y:")}
            SpinBox {
                from: 0
                value: tilingParams.tileSizeY
                onValueChanged: tilingParams.tileSizeY = parseInt(value, 10)
            }
        }

        // Q_PROPERTY(float targetOverlap READ targetOverlap WRITE setTargetOverlap NOTIFY targetOverlapChanged FINAL)
        RowLayout{
            Label{text: qsTr("Target Overlap:")}
            SpinBox {
                from: 0.00
                value: tilingParams.targetOverlap
                onValueChanged: tilingParams.targetOverlap = parseFloat(value)
            }
        }

        // Q_PROPERTY(float relSizeX READ relSizeX WRITE setRelSizeX NOTIFY relSizeXChanged FINAL)
        RowLayout{
            Label{text: qsTr("Rel Size X:")}
            SpinBox {
                from: 0.00
                value: tilingParams.relSizeX
                onValueChanged: tilingParams.relSizeX = parseFloat(value)
            }
        }


        // Q_PROPERTY(float relSizeY READ relSizeY  WRITE setRelSizeY  NOTIFY relSizeYChanged FINAL)
        RowLayout{
            Label{text: qsTr("Rel Size Y:")}
            SpinBox {
                from: 0.00
                value: tilingParams.relSizeY
                onValueChanged: tilingParams.relSizeY = parseFloat(value)
            }
        }

        // Q_PROPERTY(QString extraTilingArgs READ extraTilingArgs WRITE setExtraTilingArgs NOTIFY extraTilingArgsChanged FINAL)
        RowLayout {
            Label{ text: qsTr("Prompt:") }
            TextField{ text: tilingParams.extraTilingArgs ; onAccepted: tilingParams.extraTilingArgs = text}
        }


    }
}
}