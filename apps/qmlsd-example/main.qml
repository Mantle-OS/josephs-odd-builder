import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Window
import QSd

ApplicationWindow {
    width: Screen.width / 2
    height: Screen.height / 2
    visible: true
    title: qsTr("Qsd playground")

    header: TabBar{
        id: bar
        width: parent.width

        TabButton{ text: qsTr("Context") }
        TabButton{ text: qsTr("Image Generation") }
        TabButton{ text: qsTr("Devices") }
        // presets
        TabButton{ text: qsTr("ZImage Turbo") }
        TabButton{ text: qsTr("SD3 Large") }
        TabButton{ text: qsTr("SD3 Medium") }
    }

    StackLayout{
        id: stackView
        currentIndex: bar.currentIndex
        anchors.fill: parent
        QmlSdCtxParams {
            ctxParams:  QSD.ctxParams
        }

        QmlSdImgGenParams{
            imgGenParams: QSD.imgGenParams
        }

        QmlSdBackendManager{

        }
        ZImageTurbo{

        }
        SDThree{
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
        SDThreeMedium{

        }

    }
    RowLayout{
        ProgressBar{
            from: 0.0
            to: QSD.totalSteps
            value: QSD.currentStep
        }
        Label{ text: QSD.currentStep + "/" + QSD.totalSteps + " " + QSD.progressionTime}
    }
}


