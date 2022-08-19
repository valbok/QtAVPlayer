import QtQuick 2.5
import QtMultimedia 6.2

Item {
    id: root
    property alias frame_fps: fpsTextVideo.text
    property alias qml_fps: fpsTextQML.text

    VideoOutput {
        anchors.fill: parent
        objectName: "videoOutput"
    }

    Rectangle {
        id: fps
        property color textColor: "white"
        property int textSize: 30

        border.width: 1
        border.color: "black"
        width: 5.5 * fps.textSize
        height: 2.5 * fps.textSize
        color: "black"
        opacity: 0.5
        radius: 0

        // This should ensure that the monitor is on top of all other content
        z: 999

        Text {
            id: labelText
            anchors {
                top: parent.top
                left: parent.left
                margins: 10
            }
            color: fps.textColor
            font.pixelSize: 0.6 * fps.textSize
            text: "Video FPS"
            width: fps.width - 2*anchors.margins
            elide: Text.ElideRight
        }

        Text {
            id: labelTextQML
            anchors {
                bottom: parent.bottom
                left: parent.left
                margins: 10
            }
            color: fps.textColor
            font.pixelSize: 0.6 * fps.textSize
            text: "QML FPS"
            width: fps.width - 2*anchors.margins
            elide: Text.ElideRight
        }

        Text {
            id: fpsTextVideo
            anchors {
                top: parent.top
                right: parent.right
                margins: 10
            }
            color: fps.textColor
            font.pixelSize: 0.6 * fps.textSize
        }

        Text {
            id: fpsTextQML
            anchors {
                bottom: parent.bottom
                right: parent.right
                margins: 10
            }
            color: fps.textColor
            font.pixelSize: 0.6 * fps.textSize
        }
    }
}
