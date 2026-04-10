import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    implicitWidth: Math.max(implicitContentWidth + 12, 40)
    implicitHeight: 34

    background: Rectangle {
        radius: 17
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.05)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    scale: hovered ? 1.04 : (pressed ? 0.96 : 1.0)
    Behavior on scale { NumberAnimation { duration: 150 } }

    opacity: enabled ? 1.0 : 0.5
}
