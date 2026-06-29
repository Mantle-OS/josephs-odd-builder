import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Window
import Sodium

ApplicationWindow {
    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    title: qsTr("Qt Sodium Example")

    header: TabBar{
        id: bar
        width: parent.width

        TabButton{
            text: qsTr("Secure Box")
        }
        TabButton{
            text: qsTr("Signer")
        }
        TabButton{
            text: qsTr("Hash")
        }
        TabButton{
            text: qsTr("Keys")
        }
        TabButton{
            text: qsTr("Passwod Utils")
        }
    }

    StackLayout{
        id: stackView
        currentIndex: bar.currentIndex
        anchors.fill: parent
        SecureBox{}
        CryptoSigner{}
        HashView{}
        KeyManager{}
        PasswodUtils{}
    }
}


