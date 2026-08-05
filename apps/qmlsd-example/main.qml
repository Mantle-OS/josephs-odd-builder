import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QSd

ApplicationWindow {
    id: root

    width: Screen.width / 2
    height: Screen.height / 2
    minimumWidth: 800
    minimumHeight: 600

    visible: true
    title: qsTr("QSD Playground")

    header: TabBar {
        id: tabBar
        width: parent.width

        TabButton {
            text: qsTr("Context")
        }

        TabButton {
            text: qsTr("Image Generation")
        }

        TabButton {
            text: qsTr("Devices")
        }

        TabButton {
            text: qsTr("Z-Image Turbo")
        }

        TabButton {
            text: qsTr("SD3 Large")
        }

        TabButton {
            text: qsTr("SD3 Medium")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: tabBar.currentIndex

            QmlSdCtxParams {
                ctxParams: QSD.ctxParams
            }

            QmlSdImgGenParams {
                imgGenParams: QSD.imgGenParams
            }

            QmlSdBackendManager {
            }

            ZImageTurbo {
            }

            SDThree {
            }

            SDThreeMedium {
            }
        }

        MenuSeparator {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 4
            Layout.bottomMargin: 4

            ProgressBar {
                Layout.fillWidth: true

                from: 0
                to: Math.max(1, QSD.totalSteps)
                value: QSD.currentStep
            }

            Label {
                text: qsTr("%1 / %2  %3")
                    .arg(QSD.currentStep)
                    .arg(QSD.totalSteps)
                    .arg(QSD.progressionTime)
            }
        }
    }
}