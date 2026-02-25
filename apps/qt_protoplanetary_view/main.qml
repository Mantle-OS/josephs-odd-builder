import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtGraphs
import ProtoplanetaryQml
ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("Protoplanetary Sort Mini View")

    ColumnLayout{
        id: header
        height: 72
        width: parent.width
        RowLayout{
            height: 32
            width: parent.width
            Button{
                Layout.fillWidth: true
                text: qsTr("start")
                onClicked: QPPSEngine.start()
            }
            Button{
                Layout.fillWidth: true
                text: qsTr("stop")
                onClicked: QPPSEngine.stop()
            }

            Button{
                Layout.fillWidth: true
                text: qsTr("play")
                onClicked: QPPSEngine.pause()
            }
            Button{
                Layout.fillWidth: true
                text: qsTr("pause")
                onClicked: QPPSEngine.pause()
            }
            Button{
                Layout.fillWidth: true
                text: qsTr("reset")
                onClicked: QPPSEngine.reset();
            }
        }
        RowLayout{
            height: 32
            width: parent.width
            Label{Layout.fillWidth: true;Layout.fillHeight: true;  text: qsTr("stellarMass: ") + QPPSEngine.disk.stellarMass }
            Label{Layout.fillWidth: true;Layout.fillHeight: true;  text: qsTr("stellarLuminosity: ") + QPPSEngine.disk.stellarLuminosity }
            Label{Layout.fillWidth: true;Layout.fillHeight: true;  text: qsTr("stellarTemp: ") + QPPSEngine.disk.stellarTemp }
            Label{Layout.fillWidth: true;Layout.fillHeight: true;  text: qsTr("innerBoundary: ") + QPPSEngine.zones.innerBoundaryAU }
            Label{Layout.fillWidth: true;Layout.fillHeight: true;  text: qsTr("midBoundary: ") + QPPSEngine.zones.midBoundaryAU }
        }
    }

    Item { // Container for the 3D chart
        id: particleViewer
        width: parent.width
        height: parent.height - header.height
        anchors.bottom: parent.bottom

        // QtGraphs 3D Scatter Plot for Visualization
        Scatter3D {
            anchors.fill: parent
            axisX.title: "Particle Radius (m)"
            axisY.title: "Particle Mass (kg)"
            axisZ.title: "Temperature (K)"
            shadowQuality: Graphs3D.ShadowQuality.High
            cameraPreset: Graphs3D.CameraPreset.Front
            theme: themeQt

            axisX.segmentCount: 3
            axisX.subSegmentCount: 2
            axisX.labelFormat: "%.2f"
            axisZ.segmentCount: 2
            axisZ.subSegmentCount: 2
            axisZ.labelFormat: "%.2f"
            axisY.segmentCount: 2
            axisY.subSegmentCount: 2
            axisY.labelFormat: "%.2f"

            Scatter3DSeries {
                id: particleSeries
                // mesh: Abstract3DSeries.Mesh.Cube
                itemLabelFormat: "Series 2: X:@xLabel Y:@yLabel Z:@zLabel"
                itemSize: 0.05
                ItemModelScatterDataProxy {
                    itemModel: QPPSEngine.particles
                    xPosRole: "posX"
                    yPosRole: "posY"
                    zPosRole: "posZ"
                }
            }
        }
    }
}
