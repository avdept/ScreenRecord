import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    implicitHeight: 34
    leftPadding: 10
    rightPadding: 10

    background: Rectangle {
        radius: 8
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.05)
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    scale: hovered ? 1.04 : (pressed ? 0.96 : 1.0)
    Behavior on scale { NumberAnimation { duration: 150 } }

    opacity: enabled ? 1.0 : 0.5
}
