import QtQuick
import QtQuick.Controls
import ScreenCopy 1.0

Item {
    id: root

    property alias videoPlayer: player

    readonly property string resBase: "qrc:/qt/qml/ScreenCopy/resources/wallpapers/"
    readonly property real paddingScale: 1.0 - (GlobalSettings.padding / 100.0) * 0.4
    readonly property real videoW: width * paddingScale
    readonly property real videoH: height * paddingScale
    readonly property real si: GlobalSettings.shadowIntensity

    // Background layer
    Rectangle {
        id: backgroundRect
        anchors.fill: parent
        radius: 12
        clip: true
        color: GlobalSettings.wallpaper.startsWith("#") ? GlobalSettings.wallpaper : Theme.background

        Image {
            anchors.fill: parent
            visible: !GlobalSettings.wallpaper.startsWith("#") && GlobalSettings.wallpaper.length > 0
            source: {
                if (GlobalSettings.wallpaper.startsWith("#") || GlobalSettings.wallpaper.length === 0)
                    return ""
                return resBase + GlobalSettings.wallpaper + ".jpg"
            }
            fillMode: Image.PreserveAspectCrop
            smooth: true
        }

        // Shadow — uses same algorithm as export (CompositeFilter)
        ShadowRenderer {
            anchors.fill: parent
            visible: root.si > 0.01
            shadowIntensity: root.si
            borderRadius: GlobalSettings.borderRadius
            videoWidth: videoContainer.width
            videoHeight: videoContainer.height
            videoX: videoContainer.x
            videoY: videoContainer.y
        }

        // Video container
        Item {
            id: videoContainer
            width: root.videoW
            height: root.videoH
            anchors.centerIn: parent

            Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on height { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

            VideoPlayer {
                id: player
                anchors.fill: parent
                borderRadius: GlobalSettings.borderRadius

                onPositionChanged: Playback.currentTimeMs = position
                onDurationChanged: Playback.durationMs = duration
                onPlayingChanged: {
                    if (playing !== Playback.playing) {
                        if (playing) Playback.play()
                        else Playback.pause()
                    }
                }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: qsTr("No video loaded")
        font.pixelSize: 14
        font.weight: Font.Medium
        color: Qt.rgba(1, 1, 1, 0.25)
        visible: !player.loaded
    }
}
