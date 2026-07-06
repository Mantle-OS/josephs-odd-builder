// ZStdOptionsPopup.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

Popup {
    id: root
    property QtObject target
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    width: 420
    height: Math.min(360, contentColumn.implicitHeight + 32)
    ScrollView {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            id: contentColumn
            width: root.availableWidth
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.target && root.target.input !== "" ? root.target.input : qsTr("No input selected")
                }
                Button {
                    text: qsTr("Choose input")
                    onClicked: inputPicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                    text: root.target && root.target.output !== "" ? root.target.output : qsTr("No output selected")
                }
                Button {
                    text: qsTr("Choose output")
                    onClicked: outputPicker.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { text: qsTr("Compression level (%1):").arg(Math.round(levelSlider.value)) }
                Slider {
                    id: levelSlider
                    Layout.fillWidth: true
                    from: 0; to: 22; stepSize: 1
                    value: root.target ? root.target.compressionLevel : 9
                    onValueChanged: if (root.target) root.target.compressionLevel = Math.round(value)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                CheckBox {
                    text: qsTr("Empty directories")
                    checked: root.target ? root.target.preserveEmptyDirectories : true
                    onCheckedChanged: if (root.target) root.target.preserveEmptyDirectories = checked
                }
                CheckBox {
                    text: qsTr("Symlinks")
                    checked: root.target ? root.target.preserveSymlinks : true
                    onCheckedChanged: if (root.target) root.target.preserveSymlinks = checked
                }
                CheckBox {
                    text: qsTr("Recurse subdirectories")
                    checked: root.target ? root.target.recursiveDirectories : true
                    onCheckedChanged: if (root.target) root.target.recursiveDirectories = checked
                }
            }

            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                readOnly: true
                wrapMode: TextArea.Wrap
                text: root.target ? root.target.errorString : ""
                visible: text !== ""
                color: "red"
            }

            Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Done")
                onClicked: root.close()
            }
        }
    }

    FileDialog {
        id: inputPicker
        onAccepted: {
            let p = selectedFile.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            if (root.target) root.target.input = p
        }
    }

    FileDialog {
        id: outputPicker
        fileMode: FileDialog.SaveFile
        onAccepted: {
            let p = selectedFile.toString()
            if (p.startsWith("file://")) p = p.replace("file://", "")
            if (root.target) root.target.output = p
        }
    }
}