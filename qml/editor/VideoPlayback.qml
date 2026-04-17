import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import ScreenCopy 1.0

Item {
    id: root

    property alias videoPlayer: player

    readonly property string resBase: "qrc:/ScreenCopy/resources/wallpapers/"
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
        color: GlobalSettings.wallpaper.startsWith("#") ? GlobalSettings.wallpaper : "#1a1a2e"

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

        // Shadow
        MultiEffect {
            anchors.fill: videoContainer
            source: videoContainer
            visible: root.si > 0.01

            shadowEnabled: true
            shadowColor: "#000000"
            shadowBlur: root.si
            shadowVerticalOffset: root.si * 14
            shadowHorizontalOffset: 0
            shadowOpacity: root.si * 0.7
            shadowScale: 1.0 + root.si * 0.05
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
