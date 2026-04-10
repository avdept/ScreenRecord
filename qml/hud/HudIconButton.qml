import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    property string iconSource: ""
    property color iconColor: Qt.rgba(1, 1, 1, 0.6)
    property bool active: false
    property bool small: false

    implicitWidth: small ? 26 : 30
    implicitHeight: small ? 26 : 30

    background: Rectangle {
        radius: width / 2
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: Image {
        source: root.iconSource
        sourceSize: Qt.size(root.small ? 14 : 16, root.small ? 14 : 16)
        fillMode: Image.PreserveAspectFit
        smooth: true
        opacity: root.enabled ? 1.0 : 0.3

        // Green glow overlay for active state
        layer.enabled: root.active
        layer.samplerName: "source"
    }

    scale: hovered ? 1.08 : (pressed ? 0.95 : 1.0)
    Behavior on scale { NumberAnimation { duration: 150 } }

    opacity: enabled ? 1.0 : 0.5

    ToolTip.visible: hovered && ToolTip.text.length > 0
    ToolTip.delay: 200
}
