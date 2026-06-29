import QtQuick
import QtQuick.Controls
import Sodium

Pane {
    id: securePane
    property string fileBackend: ""
    property string placeholderText: qsTr("Enter string...")
    property bool autoWipeOnFocusLoss: true
    property color secureActiveColor: "#00FF66"
    property color secureIdleColor:   "#555555"

    // Default dimensions/padding configuration for proper styling
    implicitWidth: 340
    implicitHeight: 60
    padding: 0
    background: Rectangle {
        color: "transparent"
        border.color: secureTextField.activeFocus ? securePane.secureActiveColor : securePane.secureIdleColor
        border.width: secureTextField.activeFocus ? 2 : 1
        radius: 4
        Behavior on border.color { ColorAnimation { duration: 150 } }
    }

    signal valueCommitted(string clearText)
    signal secureWipeExecuted()

    QmlSodiumBox {
        id: box
        password: secureTextField.text
    }

    TextField {
        id: secureTextField
        anchors.fill: parent
        anchors.margins: 2
        placeholderText: securePane.placeholderText
        echoMode: TextInput.Password
        passwordCharacter: "*"
        selectByMouse: true
        background: null
        onAccepted: {
            if (secureTextField.text.length > 0) {
                securePane.valueCommitted(secureTextField.text);
                if (securePane.autoWipeOnFocusLoss)
                    secureWipe();
            }
        }

        onActiveFocusChanged: {
            if (!secureTextField.activeFocus && securePane.autoWipeOnFocusLoss) {
                if (secureTextField.text.length > 0)
                    securePane.valueCommitted(secureTextField.text);
                secureWipe();
            }
        }

        function secureWipe() {
            secureTextField.text = "";
            secureTextField.deselect();
            securePane.secureWipeExecuted();
        }
    }
}