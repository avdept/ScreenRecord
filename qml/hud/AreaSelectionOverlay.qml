import QtQuick
import QtQuick.Controls

Window {
    id: overlay
    visible: false
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    x: 0; y: 0
    width: Screen.width
    height: Screen.height

    signal areaSelected(int x, int y, int w, int h)
    signal cancelled()

    property point startPos: Qt.point(0, 0)
    property rect selection: Qt.rect(0, 0, 0, 0)
    property bool selecting: false

    function normalizedRect(p1, p2) {
        var x = Math.min(p1.x, p2.x)
        var y = Math.min(p1.y, p2.y)
        var w = Math.abs(p2.x - p1.x)
        var h = Math.abs(p2.y - p1.y)
        return Qt.rect(x, y, w, h)
    }

    // Dim overlay with cutout
    Item {
        anchors.fill: parent

        // Top
        Rectangle {
            x: 0; y: 0
            width: parent.width
            height: selection.y
            color: Qt.rgba(0, 0, 0, 0.4)
            visible: selecting
        }
        // Bottom
        Rectangle {
            x: 0; y: selection.y + selection.height
            width: parent.width
            height: parent.height - selection.y - selection.height
            color: Qt.rgba(0, 0, 0, 0.4)
            visible: selecting
        }
        // Left
        Rectangle {
            x: 0; y: selection.y
            width: selection.x
            height: selection.height
            color: Qt.rgba(0, 0, 0, 0.4)
            visible: selecting
        }
        // Right
        Rectangle {
            x: selection.x + selection.width; y: selection.y
            width: parent.width - selection.x - selection.width
            height: selection.height
            color: Qt.rgba(0, 0, 0, 0.4)
            visible: selecting
        }

        // Full dim when not yet dragging
        Rectangle {
            anchors.fill: parent
            color: Qt.rgba(0, 0, 0, 0.4)
            visible: !selecting
        }

        // Selection border
        Rectangle {
            x: selection.x; y: selection.y
            width: selection.width; height: selection.height
            color: "transparent"
            border.color: "white"
            border.width: 2
            visible: selecting && selection.width > 0 && selection.height > 0
        }

        // Dimension label
        Rectangle {
            visible: selecting && selection.width > 10 && selection.height > 10
            x: selection.x + selection.width / 2 - width / 2
            y: selection.y + selection.height + 8
            width: dimLabel.implicitWidth + 16
            height: dimLabel.implicitHeight + 8
            radius: 4
            color: Qt.rgba(0, 0, 0, 0.75)

            Text {
                id: dimLabel
                anchors.centerIn: parent
                text: Math.round(selection.width) + " x " + Math.round(selection.height)
                font.pixelSize: 12
                color: "white"
            }
        }

        // Crosshair cursor hint
        Text {
            visible: !selecting
            anchors.centerIn: parent
            text: qsTr("Click and drag to select an area. Press Escape to cancel.")
            font.pixelSize: 16
            color: Qt.rgba(1, 1, 1, 0.8)
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.CrossCursor

        onPressed: function(mouse) {
            startPos = Qt.point(mouse.x, mouse.y)
            selection = Qt.rect(mouse.x, mouse.y, 0, 0)
            selecting = true
        }

        onPositionChanged: function(mouse) {
            if (selecting) {
                selection = normalizedRect(startPos, Qt.point(mouse.x, mouse.y))
            }
        }

        onReleased: function(mouse) {
            if (selecting) {
                selection = normalizedRect(startPos, Qt.point(mouse.x, mouse.y))
                selecting = false

                if (selection.width > 10 && selection.height > 10) {
                    overlay.areaSelected(
                        Math.round(selection.x),
                        Math.round(selection.y),
                        Math.round(selection.width),
                        Math.round(selection.height)
                    )
                } else {
                    overlay.cancelled()
                }

                overlay.close()
            }
        }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            selecting = false
            overlay.cancelled()
            overlay.close()
        }
    }
}
