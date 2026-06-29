import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd
Pane {
    property QSdPmParams pmParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Pm Settings") }
        MenuSeparator{Layout.fillWidth: true  }
        RowLayout{
            // Q_PROPERTY(QString idEmbedPath READ idEmbedPath WRITE setIdEmbedPath NOTIFY idEmbedPathChanged FINAL)
            RowLayout{
                Label{text: qsTr("Id Embed Path:")}
                TextField{readOnly: true; text: pmParams.idEmbedPath ; Layout.fillWidth: true }
                Button{
                    text: qsTr("Change") ;
                    onClicked: {
                        fileManager.open();
                        openFileManager("idEmbedPath")
                    }
                }
            }
            // Q_PROPERTY(int idImagesCount READ idImagesCount WRITE setIdImagesCount NOTIFY idImagesCountChanged FINAL)
            RowLayout{
                Label{text: qsTr("Id Images Count:")}
                SpinBox{
                    from: 0
                    value: pmParams.idImagesCount
                    onValueChanged: pmParams.idImages = parseInt(value, 10)
                }
            }
            // Q_PROPERTY(float styleStrength READ styleStrength WRITE setStyleStrength NOTIFY styleStrengthChanged FINAL)
            RowLayout{
                Label{text: qsTr("Style Strength:")}
                SpinBox{
                    from: 0.00
                    value: pmParams.styleStrength
                    onValueChanged: pmParams.styleStrength = parseFloat(value)
                }
            }
        }
    }
}