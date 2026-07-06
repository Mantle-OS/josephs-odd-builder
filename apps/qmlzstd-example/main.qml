import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import QtQuick.Window
import QZstd

ApplicationWindow {
    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    title: qsTr("Qt Zstd example")

    header: TabBar{
        id: bar
        width: parent.width

        TabButton{
            text: qsTr("Async")
        }
        TabButton{
            text: qsTr("Blocking Compress")
        }
        TabButton{
            text: qsTr("Blocking Decompress")
        }
        TabButton{
            text: qsTr("Blocking ZstdCrypto")
        }
        TabButton{
            text: qsTr("About")
        }
    }

    StackLayout{
        id: stackView
        currentIndex: bar.currentIndex
        anchors.fill: parent
        ZStdPage{} //Async
        ZStdCompressPage{} // Vlocking
        ZStdDecompressPage{} // blocking
        ZstdCrypto{}
        About{}
    }
}


