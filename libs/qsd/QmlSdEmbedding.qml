import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QSd
Pane {
    property QSdEmbedding embeddingParams: null
    ColumnLayout{
        Layout.fillHeight: true
        Layout.fillWidth: true
        Label{ text: qsTr("Guidance Settings") }
        MenuSeparator{Layout.fillWidth: true  }

        RowLayout{
            // Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
            RowLayout{
                Label{text: qsTr("Name:")}
                TextField {
                    text: QSD.ContextParams.embeddings.name
                    onAccepted:  QSD.ContextParams.embeddings.name = text
                }
            }

            // Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged FINAL)
            RowLayout{
                Label{text: qsTr("Path:")}
                TextField {
                    text: QSD.ContextParams.embeddings.path
                    onAccepted:  QSD.ContextParams.embeddings.path = text
                }
            }
        }
    }
}
