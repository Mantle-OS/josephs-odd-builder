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
        id: tb
        width: parent.width
        TabButton{ text: qsTr("Secure Box") }
        TabButton{ text: qsTr("Signer") }
        TabButton{ text: qsTr("Hash") }
        TabButton{ text: qsTr("Keys") }
        TabButton{ text: qsTr("Password Utils") }
    }    
    StackLayout{
        currentIndex: tb.currentIndex
        anchors.fill: parent
        SecureBox{}
        CryptoSigner{}
        HashView{}
        KeyManager{}
        PasswordUtils{}
    }
}
