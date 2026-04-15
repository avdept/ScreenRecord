import QtQuick
import QtQuick.Controls

AbstractButton {
    id: root

    property string iconSource: ""
    property color iconColor: Qt.rgba(1, 1, 1, 0.6)
    property bool active: false
    property bool small: false

    implicitWidth: small ? 26 : 34
    implicitHeight: small ? 26 : 34

    background: Rectangle {
        radius: 8
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: Item {
        implicitWidth: root.small ? 14 : 18
        implicitHeight: root.small ? 14 : 18

        Image {
            anchors.centerIn: parent
            width: root.small ? 14 : 18
            height: root.small ? 14 : 18
            source: root.iconSource
            // Render at 2x for crisp display, scale down
            sourceSize: Qt.size(48, 48)
            fillMode: Image.PreserveAspectFit
            smooth: true
            opacity: root.enabled ? 1.0 : 0.3

            layer.enabled: root.active
            layer.samplerName: "source"
        }
    }

    scale: hovered ? 1.08 : (pressed ? 0.95 : 1.0)
    Behavior on scale { NumberAnimation { duration: 150 } }

    opacity: enabled ? 1.0 : 0.5

    ToolTip.visible: hovered && ToolTip.text.length > 0
    ToolTip.delay: 200
}
