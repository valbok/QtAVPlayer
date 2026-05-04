import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtMultimedia

ApplicationWindow {
    id: root
    title: "QtAVPlayer"
    width: 960
    height: 600
    visible: true
    color: "#0d0d0d"

    // Accessing it through `pc` with a fallback object ensures
    // no binding ever receives null and throws a TypeError.
    readonly property var pc: playerController ?? ({
        hasMedia: false, playing: false,
        position: 0, duration: 0, volume: 1.0, errorString: "", subtitleTracks: [], audioTracks: [], videoTracks: []
    })

    readonly property color accentColor:  "#e8c84a"   // warm amber
    readonly property color bgDark:       "#0d0d0d"
    readonly property color bgPanel:      "#161616"
    readonly property color bgControl:    "#1f1f1f"
    readonly property color textPrimary:  "#f0ede6"
    readonly property color textMuted:    "#666"
    readonly property int   radius:       6
    property string selectedSubtitleStreamIndex: "-1"
    property string selectedAudioStreamIndex:    "-1"
    property string selectedVideoStreamIndex:    "-1"
    property bool softwareVideoCodec:            false
    property bool copyFreeRender:                true

    function formatTime(ms) {
        if (ms <= 0) return "0:00"
        var totalSec = Math.floor(ms / 1000)
        var h = Math.floor(totalSec / 3600)
        var m = Math.floor((totalSec % 3600) / 60)
        var s = totalSec % 60
        var mm = m < 10 ? "0" + m : m
        var ss = s < 10 ? "0" + s : s
        return h > 0 ? h + ":" + mm + ":" + ss : m + ":" + ss
    }

    FileDialog {
        id: fileDialog
        title: "Open Media File"
        nameFilters: [
            "Video files (*.mp4 *.mkv *.avi *.mov *.webm *.flv *.wmv *.ts *.m2ts)",
            "Audio files (*.mp3 *.aac *.flac *.ogg *.wav)",
            "All files (*)"
        ]
        onAccepted: { playerController.openFile(selectedFile); root.title = selectedFile }
    }

    Dialog {
        id: urlDialog
        title: "Open URL"
        width: 500
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (urlField.text.trim() !== "")
                playerController.openUrl(urlField.text.trim())
        }

        Column {
            width: parent.width
            spacing: 12

            Text {
                text: "Enter media URL:"
                font.pixelSize: 13
            }

            TextField {
                id: urlField
                width: parent.width
                placeholderText: "https://  or  rtsp://  or  /dev/video0"
                color: root.textPrimary
                background: Rectangle {
                    color: root.bgControl
                    radius: root.radius
                    border.color: urlField.activeFocus ? root.accentColor : "#333"
                    border.width: 1
                }
                font.pixelSize: 13
                padding: 8
                onAccepted: urlDialog.accept()
            }
        }
    }

    Rectangle {
        id: errorBar
        anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 56 + 28 + 8 }
        z: 100 // Ensure it's on top
        // 56 = controlBar height, 28 = seekBar height, 8 = gap
        width: errorText.implicitWidth + 32
        height: 36
        radius: root.radius
        color: "#c0392b"
        visible: opacity > 0
        opacity: 0

        Text {
            id: errorText
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 13
        }

        Behavior on opacity { NumberAnimation { duration: 250 } }

        function show(msg) {
            errorText.text = msg
            console.log(msg)
            opacity = 1
            hideTimer.restart()
        }
        Timer {
            id: hideTimer
            interval: 4000
            onTriggered: errorBar.opacity = 0
        }
    }

    Connections {
        target: playerController
        function onErrorOccurred() { errorBar.show(playerController.errorString) }
        function onSubtitleTracksChanged() {
            subtitleMenuModel.clear()
            subtitleMenuModel.append({"name": "Disable", "streamIndex": "-1"});
            for (var i in playerController.subtitleTracks) {
                subtitleMenuModel.append({"name": playerController.subtitleTracks[i], "streamIndex": i});
            }
        }
        function onSubtitleTrackChanged(idx) {
            selectedSubtitleStreamIndex = idx;
        }
        function onSubtitleTextChanged(text, ms) {
            subtitle.text = text
            subtitle.visible = true
            subtitleTimer.interval = ms
            subtitleTimer.restart()
        }
        function onAudioTracksChanged() {
            audioMenuModel.clear()
            for (var i in playerController.audioTracks) {
                audioMenuModel.append({"name": playerController.audioTracks[i], "streamIndex": i});
            }
        }
        function onAudioTrackChanged(idx) {
            selectedAudioStreamIndex = idx;
        }
        function onVideoTracksChanged() {
            videoMenuModel.clear()
            for (var i in playerController.videoTracks) {
                videoMenuModel.append({"name": playerController.videoTracks[i], "streamIndex": i});
            }
        }
        function onVideoTrackChanged(idx) {
            selectedVideoStreamIndex = idx;
        }
        function onHwDeviceChanged(hw) {
            softwareVideoCodec = !hw;
            copyFreeMenu.enabled = !softwareVideoCodec;
            softwareVideoCodecMenu.enabled = hw
        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (drop.hasUrls)
                playerController.openFile(drop.urls[0])
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: root.bgPanel

            RowLayout {
                anchors { left: parent.left; verticalCenter: parent.verticalCenter; leftMargin: 12 }
                spacing: 2

                MenuBarButton {
                    id: fileMenuButton
                    text: "File  ▾"
                    highlighted: fileMenu.visible

                    onClicked: fileMenu.visible ? fileMenu.close() : fileMenu.open()

                    Menu {
                        id: fileMenu
                        // Open downward from the button
                        y: fileMenuButton.height

                        background: Rectangle {
                            color: root.bgPanel
                            border.color: root.accentColor
                            border.width: 1
                            radius: root.radius
                            implicitWidth: 200
                        }

                        MenuItem {
                            text: "Open File"
                            onTriggered: fileDialog.open()
                            contentItem: Text {
                                leftPadding: 12
                                text: parent.text
                                color: parent.highlighted ? root.accentColor : root.textPrimary
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                            }
                        }
                        MenuItem {
                            text: "Open URL"
                            onTriggered: { urlField.text = ""; urlDialog.open() }
                            contentItem: Text {
                                leftPadding: 12
                                text: parent.text
                                color: parent.highlighted ? root.accentColor : root.textPrimary
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                            }
                        }
                        MenuSeparator {
                            contentItem: Rectangle {
                                implicitHeight: 1
                                color: "#333"
                            }
                        }
                        MenuItem {
                            text: "Quit"
                            onTriggered: Qt.quit()
                            contentItem: Text {
                                leftPadding: 12
                                text: parent.text
                                color: parent.highlighted ? root.accentColor : root.textPrimary
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                            }
                        }
                    }
                }
                MenuBarButton {
                    id: subtitleMenuButton
                    text: "Subtitle ▾"
                    highlighted: subtitleMenu.visible
                    enabled: pc.subtitleTracks.length > 0
                    onClicked: subtitleMenu.visible ? subtitleMenu.close() : subtitleMenu.open()

                    ListModel {
                        id: subtitleMenuModel
                    }

                    Menu {
                        id: subtitleMenu
                        // Open downward from the button
                        y: subtitleMenuButton.height

                        background: Rectangle {
                            color: root.bgPanel
                            border.color: root.accentColor
                            border.width: 1
                            radius: root.radius
                            implicitWidth: 200
                        }

                        Instantiator {
                            model: subtitleMenuModel
                            delegate: MenuItem {
                                text: model.name
                                contentItem: Text {
                                    leftPadding: 12
                                    text: parent.text
                                    color: streamIndex == selectedSubtitleStreamIndex ? root.accentColor
                                         : parent.highlighted                         ? root.textPrimary
                                         :                                              root.textMuted
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                                }
                                onTriggered: {
                                    pc.setSubtitleTrack(streamIndex)
                                }
                            }
                            // Manually add/remove created objects to the Menu
                            onObjectAdded: (index, object) => subtitleMenu.insertItem(index, object)
                            onObjectRemoved: (index, object) => subtitleMenu.removeItem(object)
                        }
                    }
                }

                MenuBarButton {
                    id: audioMenuButton
                    text: "Audio ▾"
                    highlighted: audioMenu.visible
                    enabled: pc.audioTracks.length > 0
                    onClicked: audioMenu.visible ? audioMenu.close() : audioMenu.open()

                    ListModel {
                        id: audioMenuModel
                    }

                    Menu {
                        id: audioMenu
                        // Open downward from the button
                        y: audioMenuButton.height

                        background: Rectangle {
                            color: root.bgPanel
                            border.color: root.accentColor
                            border.width: 1
                            radius: root.radius
                            implicitWidth: 200
                        }

                        Instantiator {
                            model: audioMenuModel
                            delegate: MenuItem {
                                text: model.name
                                contentItem: Text {
                                    leftPadding: 12
                                    text: parent.text
                                    color: streamIndex == selectedAudioStreamIndex ? root.accentColor
                                         : parent.highlighted                      ? root.textPrimary
                                         :                                           root.textMuted
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                                }
                                onTriggered: {
                                    pc.setAudioTrack(streamIndex)
                                }
                            }
                            // Manually add/remove created objects to the Menu
                            onObjectAdded: (index, object) => audioMenu.insertItem(index, object)
                            onObjectRemoved: (index, object) => audioMenu.removeItem(object)
                        }
                    }
                }

                MenuBarButton {
                    id: videoMenuButton
                    text: "Video ▾"
                    highlighted: videoMenu.visible
                    enabled: pc.videoTracks.length > 0
                    onClicked: videoMenu.visible ? videoMenu.close() : videoMenu.open()

                    ListModel {
                        id: videoMenuModel
                    }

                    Menu {
                        id: videoMenu
                        // Open downward from the button
                        y: videoMenuButton.height

                        background: Rectangle {
                            color: root.bgPanel
                            border.color: root.accentColor
                            border.width: 1
                            radius: root.radius
                            implicitWidth: 200
                        }

                        MenuSeparator {
                            contentItem: Rectangle {
                                implicitHeight: 1
                                color: "#333"
                            }
                        }

                        MenuItem {
                            id: softwareVideoCodecMenu
                            contentItem: Text {
                                leftPadding: 12
                                text: "Software video codec"
                                color: parent.enabled
                                        ? softwareVideoCodec
                                            ? root.accentColor
                                                : parent.highlighted
                                                    ? root.textPrimary
                                                    : root.textMuted
                                            : root.textMuted
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                            }
                            onTriggered: {
                                softwareVideoCodec = !softwareVideoCodec;
                                pc.setVideoCodec(softwareVideoCodec ? "software" : "");
                                copyFreeMenu.enabled = !softwareVideoCodec;
                            }
                        }

                        MenuItem {
                            id: copyFreeMenu
                            contentItem: Text {
                                leftPadding: 12
                                text: "Copy-free render"
                                color: parent.enabled
                                        ? copyFreeRender
                                            ? root.accentColor
                                            : parent.highlighted
                                                ? root.textPrimary
                                                : root.textMuted
                                        : root.textMuted
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                            }
                            onTriggered: {
                                copyFreeRender = !copyFreeRender;
                                pc.setCopyFreeRender(copyFreeRender);
                            }
                        }

                        Instantiator {
                            model: videoMenuModel
                            delegate: MenuItem {
                                text: model.name
                                contentItem: Text {
                                    leftPadding: 12
                                    text: parent.text
                                    color: streamIndex == selectedVideoStreamIndex ? root.accentColor
                                         : parent.highlighted                      ? root.textPrimary
                                         :                                           root.textMuted
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    color: parent.highlighted ? Qt.rgba(1,1,1,0.06) : "transparent"
                                }
                                onTriggered: {
                                    pc.setVideoTrack(streamIndex)
                                }
                            }
                            // Manually add/remove created objects to the Menu
                            onObjectAdded: (index, object) => videoMenu.insertItem(index, object)
                            onObjectRemoved: (index, object) => videoMenu.removeItem(object)
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "black"

            // Placeholder when no media is loaded
            Column {
                anchors.centerIn: parent
                spacing: 16
                visible: !pc.hasMedia

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "▶"
                    font.pixelSize: 64
                    color: "#222"
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Drop a file or use Open File / Open URL"
                    color: root.textMuted
                    font.pixelSize: 14
                }
            }

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                visible: pc.hasMedia

                Component.onCompleted: {
                    playerController.setVideoSink(videoOutput.videoSink)
                }
            }

            // Double-click to toggle fullscreen
            MouseArea {
                anchors.fill: parent
                onDoubleClicked: root.visibility === Window.FullScreen
                                     ? root.showNormal()
                                     : root.showFullScreen()
                onClicked: playerController.hasMedia
                               ? (playerController.playing
                                      ? playerController.pause()
                                      : playerController.play())
                               : fileDialog.open()
            }
            Text {
                id: subtitle
                anchors {
                    bottom: parent.bottom
                    horizontalCenter: parent.horizontalCenter
                    bottomMargin: 18
                    left: parent.left
                    leftMargin: 32
                    rightMargin: 32
                }
                text: ""
                color: "white"
                font.pixelSize: 22
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                style: Text.Outline
                styleColor: "#000"
                // Subtle semi-transparent backing for readability
                Rectangle {
                    anchors { fill: parent; margins: -4 }
                    color: "#00000000"
                    radius: 4
                    z: -1
                }
            }
            Timer {
                id: subtitleTimer
                interval: 4000
                onTriggered: subtitle.visible = false
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: root.bgPanel

            RowLayout {
                anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
                spacing: 10

                Text {
                    text: formatTime(pc.position)
                    color: root.textMuted
                    font.pixelSize: 11
                    font.family: "monospace"
                }

                SeekSlider {
                    id: seekBar
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, pc.duration)
                    value: pc.position
                    enabled: pc.hasMedia
                    accentColor: root.accentColor

                    onMoved: pc.seek(value)
                }

                Text {
                    text: formatTime(pc.duration)
                    color: root.textMuted
                    font.pixelSize: 11
                    font.family: "monospace"
                }
            }
        }


        Rectangle {
            id: controlBar
            Layout.fillWidth: true
            height: 56
            color: root.bgPanel

            RowLayout {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16 }
                spacing: 8

                // Stop
                ControlButton {
                    text: "⏹"
                    tooltip: "Stop"
                    onClicked: pc.stop()
                    enabled: pc.hasMedia
                }

                // Step backward
                ControlButton {
                    text: "⏮"
                    tooltip: "Step backward"
                    onClicked: pc.stepBackward()
                    enabled: pc.hasMedia
                }

                // Play / Pause
                ControlButton {
                    text: pc.playing ? "⏸" : "▶"
                    tooltip: pc.playing ? "Pause" : "Play"
                    highlighted: true
                    onClicked: pc.playing
                                   ? pc.pause()
                                   : pc.play()
                    enabled: pc.hasMedia
                }

                // Step forward
                ControlButton {
                    text: "⏭"
                    tooltip: "Step forward"
                    onClicked: pc.stepForward()
                    enabled: pc.hasMedia
                }

                Item { Layout.fillWidth: true }

                // Volume icon
                Text {
                    text: pc.volume === 0 ? "🔇"
                        : pc.volume < 0.4 ? "🔈"
                        : pc.volume < 0.8 ? "🔉" : "🔊"
                    font.pixelSize: 18
                    color: root.textPrimary
                }

                // Volume slider
                Slider {
                    id: volumeSlider
                    from: 0; to: 1
                    value: pc.volume
                    implicitWidth: 100
                    onMoved: pc.volume = value

                    background: Rectangle {
                        x: volumeSlider.leftPadding
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        implicitWidth: 100; implicitHeight: 4
                        width: volumeSlider.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#333"

                        Rectangle {
                            width: volumeSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: root.accentColor
                        }
                    }

                    handle: Rectangle {
                        x: volumeSlider.leftPadding + volumeSlider.visualPosition
                           * (volumeSlider.availableWidth - width)
                        y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                        width: 14; height: 14; radius: 7
                        color: root.accentColor
                    }
                }

                // Fullscreen toggle
                ControlButton {
                    text: "⛶"
                    tooltip: "Toggle fullscreen"
                    onClicked: root.visibility === Window.FullScreen
                                   ? root.showNormal()
                                   : root.showFullScreen()
                }
            }
        }
    }

    Shortcut {
        // Use standardQuit for cross-platform (Ctrl+Q on Linux/Win, Cmd+Q on macOS)
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    Shortcut {
        sequence: "Esc"
        context: Qt.ApplicationShortcut
        onActivated: {
            if (root.visibility === Window.FullScreen)
                root.showNormal()
            else if (urlDialog.visible)
                urlDialog.reject()
            else
                Qt.quit()
        }
    }

    Shortcut {
        sequence: "F"
        context: Qt.ApplicationShortcut
        onActivated: root.visibility === Window.FullScreen ? root.showNormal() : root.showFullScreen()
    }

    Shortcut {
        sequence: "Space"
        context: Qt.ApplicationShortcut
        onActivated: playerController.playing ? playerController.pause() : playerController.play()
    }

    Shortcut {
        sequence: "Left"
        context: Qt.ApplicationShortcut
        onActivated: playerController.seek(Math.max(0, playerController.position - 5000))
    }

    Shortcut {
        sequence: "Right"
        context: Qt.ApplicationShortcut
        onActivated: playerController.seek(Math.min(playerController.duration, playerController.position + 5000))
    }

    Shortcut {
        sequence: "Down"
        context: Qt.ApplicationShortcut
        onActivated: playerController.volume = Math.max(0.0, playerController.volume - 0.1)
    }

    Shortcut {
        sequence: "Up"
        context: Qt.ApplicationShortcut
        onActivated: playerController.volume = Math.min(1.0, playerController.volume + 0.1)
    }

    Shortcut {
        sequence: "Ctrl+O"
        context: Qt.ApplicationShortcut
        onActivated: fileDialog.open()
    }

    component MenuBarButton: AbstractButton {
        property bool highlighted: false
        implicitHeight: 28
        leftPadding: 12; rightPadding: 12

        contentItem: Text {
            text: parent.text
            color: parent.enabled ? (parent.highlighted || parent.hovered ? root.accentColor : root.textPrimary) : root.textMuted
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        background: Rectangle {
            color: parent.highlighted ? Qt.rgba(1,1,1,0.08)
                 : parent.hovered     ? Qt.rgba(1,1,1,0.05)
                 : "transparent"
            radius: root.radius
        }
    }

    component ControlButton: AbstractButton {
        property bool highlighted: false
        property string tooltip: ""
        implicitWidth: 38; implicitHeight: 38

        ToolTip.visible: hovered && tooltip !== ""
        ToolTip.text: tooltip
        ToolTip.delay: 600

        contentItem: Text {
            text: parent.text
            font.pixelSize: parent.highlighted ? 22 : 18
            color: !parent.enabled     ? root.textMuted
                 : parent.highlighted  ? root.accentColor
                 : parent.hovered      ? root.textPrimary
                 :                       "#aaa"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment:   Text.AlignVCenter
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        background: Rectangle {
            color: parent.pressed ? Qt.rgba(1,1,1,0.08)
                 : parent.hovered  ? Qt.rgba(1,1,1,0.04)
                 :                   "transparent"
            radius: root.radius
        }
    }

    component SeekSlider: Slider {
        property color accentColor: "white"

        background: Rectangle {
            x: parent.leftPadding
            y: parent.topPadding + parent.availableHeight / 2 - height / 2
            implicitWidth: 100; implicitHeight: 4
            width: parent.availableWidth;
            height: 4
            radius: 2
            color: "#2a2a2a"

            Rectangle {
                width: parent.parent.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: accentColor
            }
        }

        handle: Rectangle {
            x: parent.leftPadding + parent.visualPosition
               * (parent.availableWidth - width)
            y: parent.topPadding + parent.availableHeight / 2 - height / 2
            width: 14; height: 14; radius: 7
            color: accentColor
            visible: parent.hovered || parent.pressed
        }
    }
}
