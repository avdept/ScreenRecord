import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

Rectangle {
    id: root
    radius: 24
    color: Qt.rgba(0, 0, 0, 0.6)

    signal seekRequested(real ms)
    signal playPauseRequested()

    HoverHandler { id: barHover }

    // Use trimmed duration and display position
    readonly property real displayDuration: TrimModel.totalDurationMs
    readonly property real displayPosition: TrimModel.originalToDisplay(Playback.currentTimeMs)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 8

        // Play/Pause button
        Rectangle {
            id: playBtn
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            radius: 16
            color: Playback.playing ? Qt.rgba(1, 1, 1, 0.1) : "white"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.1)

            layer.enabled: !Playback.playing

            Image {
                anchors.centerIn: parent
                source: Playback.playing
                    ? "qrc:/ScreenCopy/resources/icons/pause-white.svg"
                    : "qrc:/ScreenCopy/resources/icons/play-black.svg"
                sourceSize: Qt.size(14, 14)
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.playPauseRequested()
            }

            scale: playHover.hovered ? 1.05 : 1.0
            Behavior on scale { NumberAnimation { duration: 150 } }
            HoverHandler { id: playHover }
        }

        // Current time (display/trimmed)
        Text {
            text: formatTime(root.displayPosition)
            font.pixelSize: 9
            font.weight: Font.Medium
            font.family: "monospace"
            color: "#cbd5e1"
            Layout.preferredWidth: 52
            horizontalAlignment: Text.AlignRight
        }

        // Seek bar
        Item {
            Layout.fillWidth: true
            height: 20

            // Track
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width
                height: 3
                radius: 1.5
                color: Qt.rgba(1, 1, 1, 0.1)

                // Fill
                Rectangle {
                    width: root.displayDuration > 0
                           ? parent.width * (root.displayPosition / root.displayDuration)
                           : 0
                    height: parent.height
                    radius: parent.radius
                    color: "#34B27B"
                }
            }

            // Thumb
            Rectangle {
                x: root.displayDuration > 0
                   ? (parent.width - width) * (root.displayPosition / root.displayDuration)
                   : 0
                anchors.verticalCenter: parent.verticalCenter
                width: 10
                height: 10
                radius: 5
                color: "white"

                scale: seekArea.containsMouse ? 1.15 : 1.0
                Behavior on scale { NumberAnimation { duration: 80 } }
            }

            MouseArea {
                id: seekArea
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onPressed: function(mouse) { seek(mouse.x) }
                onPositionChanged: function(mouse) { if (pressed) seek(mouse.x) }

                function seek(mouseX) {
                    var ratio = Math.max(0, Math.min(1, mouseX / width))
                    var displayMs = ratio * root.displayDuration
                    // Map display time → original video time
                    var originalMs = TrimModel.displayToOriginal(displayMs)
                    root.seekRequested(originalMs)
                }
            }
        }

        // Duration (trimmed)
        Text {
            text: formatTime(root.displayDuration)
            font.pixelSize: 9
            font.weight: Font.Medium
            font.family: "monospace"
            color: "#64748b"
            Layout.preferredWidth: 52
        }
    }

    function formatTime(ms) {
        var totalSec = Math.floor(ms / 1000)
        var min = Math.floor(totalSec / 60)
        var sec = totalSec % 60
        var millis = Math.floor(ms % 1000)
        return min.toString().padStart(2, '0') + ":" + sec.toString().padStart(2, '0') + "." + millis.toString().padStart(3, '0')
    }
}
