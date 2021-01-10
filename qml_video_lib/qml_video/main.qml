import QtQuick 2.5
import QtMultimedia 5.15

Item {
    VideoOutput {
        anchors.fill: parent
        objectName: "videoOutput"
        flushMode: VideoOutput.LastFrame
    }
}
