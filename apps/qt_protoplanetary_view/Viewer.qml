import QtQuick
import QtGraphs
import ProtoplanetaryQml
Item {
    id: chart
    width: 640
    height: 480
    Scatter3D {
        Scatter3DSeries {
            ItemModelScatterDataProxy {
                // just to test the changes
                itemModel: QPPSEngine.particles
                xPosRole: "radius"
                yPosRole: "mass"
                zPosRole: "temperature"
            }
        }
    }
}
