import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import QtQuick.Window
import QHF

ApplicationWindow {
    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    title: qsTr("Qt Huggingface hub example")

    header: TabBar{
        id: bar
        width: parent.width

        TabButton{
            text: qsTr("Trending")
        }
        TabButton{
            text: qsTr("Cache")
        }
        TabButton{
            text: qsTr("Get")
        }

    }

    StackLayout{
        id: stackView
        currentIndex: bar.currentIndex
        HFTrending{}
        HFCache{}
        HFGet{}
    }



    Popup {
        id: loginPopup
        width: parent.width * 0.98
        height: parent.height * 0.98
        anchors.centerIn:  parent
        ColumnLayout{
            anchors.fill: parent

            Label{
                id: loginFailedTxt
                visible: false
                color: "red"
                Layout.fillWidth: true
                text:qsTr("Login Failed. Please Try Again")
            }
           RowLayout{
                Label{text: qsTr("Hugging face Api Key")}
                TextField{
                    id: apiKeyTextField
                }
           }

            Button{
                Layout.fillWidth: true
                text: qsTr("login")
                // enabled: QHF.
                onClicked:{


                }
            }


                Button{
                    Layout.fillWidth: true
                    text: qsTr("Use An")
                    onClicked:{
                    }
                }
                Item{
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                }

            }
        }




}


