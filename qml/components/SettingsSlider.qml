import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string label: ""
    property real value: 0
    property real from: 0
    property real to: 1
    property real stepSize: 0.01
    property color accentColor: Theme.green

    signal moved(real value)

    Layout.fillWidth: true
    implicitHeight: col.implicitHeight

    ColumnLayout {
        id: col
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Text {
                text: root.label
                font.pixelSize: 11
                color: Theme.textMuted
                Layout.fillWidth: true
            }

            Text {
                text: root.value.toFixed(root.stepSize < 1 ? 2 : 0)
                font.pixelSize: 10
                font.family: "monospace"
                color: Theme.textDim
            }
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            from: root.from
            to: root.to
            stepSize: root.stepSize
            value: root.value

            onMoved: root.moved(slider.value)

            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                implicitWidth: 200
                implicitHeight: 4
                width: slider.availableWidth
                height: implicitHeight
                radius: 2
                color: Qt.rgba(1, 1, 1, 0.1)

                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: parent.radius
                    color: root.accentColor
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 12
                height: 12
                radius: 6
                color: Theme.white
                border.width: 2
                border.color: root.accentColor

                scale: slider.pressed ? 1.25 : (slider.hovered ? 1.15 : 1.0)
                Behavior on scale { NumberAnimation { duration: 80 } }
            }
        }
    }
}
