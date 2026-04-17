import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

Item {
    id: root
    clip: true

    signal seekRequested(real ms)

    readonly property color green: "#34B27B"
    readonly property string iconBase: "qrc:/ScreenCopy/resources/icons/"

    // Current active tool
    property string activeTool: "select"  // "select" or "trim"

    // Use trimmed (collapsed) duration for all layout
    readonly property real totalMs: TrimModel.totalDurationMs
    property real trackPadding: 4

    // Map display ms (collapsed) to pixel X
    function displayMsToX(displayMs) {
        if (totalMs <= 0) return trackPadding
        return trackPadding + (trackArea.width - trackPadding * 2) * (displayMs / totalMs)
    }

    // Map pixel X to display ms, then to original ms for seeking
    function xToOriginalMs(px) {
        if (trackArea.width - trackPadding * 2 <= 0) return 0
        var ratio = (px - trackPadding) / (trackArea.width - trackPadding * 2)
        var displayMs = Math.max(0, Math.min(totalMs, ratio * totalMs))
        return TrimModel.displayToOriginal(displayMs)
    }

    // Current playhead position in display coordinates
    readonly property real playheadDisplayMs: TrimModel.originalToDisplay(Playback.currentTimeMs)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 4

                TimelineToolButton {
                    iconSource: root.iconBase + "cursor-select.svg"
                    text: "Select"
                    shortcutKey: "S"
                    active: root.activeTool === "select"
                    onClicked: root.activeTool = "select"
                }

                TimelineToolButton {
                    iconSource: root.iconBase + "scissors.svg"
                    text: "Trim"
                    shortcutKey: "T"
                    active: root.activeTool === "trim"
                    onClicked: root.activeTool = "trim"
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.05)
            }
        }

        // Time ruler — uses trimmed duration
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#09090b"

            Item {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8

                Repeater {
                    model: {
                        var dur = root.totalMs / 1000.0
                        if (dur <= 0) return 0
                        if (dur <= 10) return Math.ceil(dur)
                        if (dur <= 60) return Math.ceil(dur / 5)
                        return Math.ceil(dur / 10)
                    }

                    Item {
                        required property int index
                        property real tickInterval: {
                            var dur = root.totalMs / 1000.0
                            if (dur <= 10) return 1
                            if (dur <= 60) return 5
                            return 10
                        }
                        property int tickCount: {
                            var dur = root.totalMs / 1000.0
                            if (dur <= 10) return Math.ceil(dur)
                            if (dur <= 60) return Math.ceil(dur / 5)
                            return Math.ceil(dur / 10)
                        }

                        x: parent.width * (index / tickCount)
                        width: 1
                        height: parent.height

                        Rectangle {
                            anchors.bottom: parent.bottom
                            width: 1
                            height: 10
                            color: Qt.rgba(1, 1, 1, 0.15)
                        }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 3
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                var sec = index * tickInterval
                                var m = Math.floor(sec / 60)
                                var s = Math.floor(sec % 60)
                                return m + ":" + s.toString().padStart(2, '0')
                            }
                            font.pixelSize: 9
                            font.family: "monospace"
                            color: Qt.rgba(1, 1, 1, 0.3)
                        }
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Qt.rgba(1, 1, 1, 0.05)
            }
        }

        // Track lane — collapsed, segments back-to-back
        Item {
            id: trackArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                anchors.fill: parent
                color: Qt.rgba(1, 1, 1, 0.015)
            }

            // Segments rendered in display (collapsed) coordinates
            Repeater {
                model: TrimModel

                Rectangle {
                    required property int index
                    required property var displayStartMs
                    required property var displayEndMs
                    required property var durationMs
                    required property var speed
                    required property bool selected

                    x: root.displayMsToX(displayStartMs)
                    width: Math.max(4, root.displayMsToX(displayEndMs) - x)
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    radius: 0
                    color: {
                        if (selected)
                            return Qt.rgba(52/255, 178/255, 123/255, 0.35)
                        if (segMouseArea.containsMouse)
                            return Qt.rgba(52/255, 178/255, 123/255, 0.22)
                        return Qt.rgba(52/255, 178/255, 123/255, 0.12)
                    }
                    border.width: selected ? 2 : 1
                    border.color: {
                        if (selected)
                            return root.green
                        if (segMouseArea.containsMouse)
                            return Qt.rgba(52/255, 178/255, 123/255, 0.5)
                        return Qt.rgba(52/255, 178/255, 123/255, 0.25)
                    }

                    Behavior on color { ColorAnimation { duration: 150 } }
                    Behavior on border.color { ColorAnimation { duration: 150 } }
                    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

                    // Speed label (when not 1x)
                    Text {
                        anchors.centerIn: parent
                        visible: speed !== 1.0 && parent.width > 40
                        text: speed.toFixed(2) + "x"
                        font.pixelSize: 9
                        font.weight: Font.Medium
                        color: "#f59e0b"
                    }

                    // Duration label
                    Text {
                        anchors.centerIn: parent
                        visible: speed === 1.0 && parent.width > 60
                        text: formatTime(durationMs)
                        font.pixelSize: 8
                        font.family: "monospace"
                        color: Qt.rgba(1, 1, 1, 0.35)
                    }

                    // Delete button
                    Rectangle {
                        visible: segMouseArea.containsMouse && TrimModel.count > 1
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.rightMargin: 4
                        anchors.topMargin: 4
                        width: 16
                        height: 16
                        radius: 8
                        color: Qt.rgba(239/255, 68/255, 68/255, 0.8)

                        Text {
                            anchors.centerIn: parent
                            text: "\u2715"
                            font.pixelSize: 9
                            font.weight: Font.Bold
                            color: "white"
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: TrimModel.removeSegment(index)
                        }
                    }

                    MouseArea {
                        id: segMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: root.activeTool === "select" ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: {
                            if (root.activeTool === "select") {
                                TrimModel.selectedIndex = index
                            }
                        }
                    }
                }
            }

            // Divider lines between segments
            Repeater {
                model: TrimModel.count > 1 ? TrimModel.count - 1 : 0

                Rectangle {
                    required property int index
                    property real divX: {
                        var seg = TrimModel.displayStartForSegment(index + 1)
                        return root.displayMsToX(seg)
                    }

                    x: divX - 1
                    width: 2
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    color: Qt.rgba(1, 1, 1, 0.15)

                    Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }
            }

            // Playhead — in display coordinates
            Rectangle {
                id: playhead
                x: root.displayMsToX(root.playheadDisplayMs) - 1
                width: 2
                height: parent.height
                color: root.green

                Rectangle {
                    anchors.centerIn: parent
                    width: 8
                    height: parent.height
                    color: Qt.rgba(52/255, 178/255, 123/255, 0.15)
                    z: -1
                }

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: -6
                    width: 12
                    height: 12
                    rotation: 45
                    radius: 2
                    color: root.green
                    border.width: 1
                    border.color: Qt.rgba(1, 1, 1, 0.2)
                }
            }

            // Track interaction — on top of everything in trim mode
            MouseArea {
                id: trackMouseArea
                anchors.fill: parent
                hoverEnabled: true
                z: root.activeTool === "trim" ? 20 : -1
                cursorShape: root.activeTool === "trim" ? Qt.BlankCursor : Qt.PointingHandCursor

                onPressed: function(mouse) {
                    if (root.activeTool === "trim") {
                        // Split at click position
                        var originalMs = root.xToOriginalMs(mouse.x)
                        TrimModel.splitAt(originalMs)
                    } else {
                        // Seek
                        seekTimeline(mouse.x)
                    }
                }

                onPositionChanged: function(mouse) {
                    if (root.activeTool === "select" && pressed) {
                        seekTimeline(mouse.x)
                    }
                }

                function seekTimeline(mouseX) {
                    var originalMs = root.xToOriginalMs(mouseX)
                    root.seekRequested(originalMs)
                }
            }

            // Trim indicator — red vertical line following mouse (rendered on top)
            Rectangle {
                id: trimIndicator
                visible: root.activeTool === "trim" && trackMouseArea.containsMouse
                x: trackMouseArea.mouseX - 1
                width: 2
                height: parent.height
                color: Qt.rgba(239/255, 68/255, 68/255, 0.8)
                z: 30

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: -4
                    width: 8
                    height: 8
                    rotation: 45
                    radius: 1
                    color: Qt.rgba(239/255, 68/255, 68/255, 0.8)
                }
            }
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
