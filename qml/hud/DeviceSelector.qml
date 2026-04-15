import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property string label: ""
    property bool showAudioMeter: false
    property real audioLevel: 0

    implicitWidth: expanded ? 240 : 160
    implicitHeight: 36
    radius: 8
    clip: true

    property bool expanded: hoverArea.containsMouse

    gradient: Gradient {
        GradientStop { position: 0.0; color: Qt.rgba(28/255, 28/255, 36/255, 0.97) }
        GradientStop { position: 1.0; color: Qt.rgba(18/255, 18/255, 26/255, 0.96) }
    }
    border.width: 1
    border.color: Qt.rgba(1, 1, 1, 0.1)

    Behavior on implicitWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }

    // Entry animation
    opacity: 0
    Component.onCompleted: opacity = 1
    Behavior on opacity { NumberAnimation { duration: 150 } }

    // Transform for slide-up
    transform: Translate {
        y: root.opacity < 1 ? 4 : 0
        Behavior on y { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 8

        Text {
            text: root.label
            font.pixelSize: 10
            font.weight: Font.Medium
            color: expanded ? Qt.rgba(1, 1, 1, 0.8) : Qt.rgba(1, 1, 1, 0.6)
            elide: Text.ElideRight
            Layout.fillWidth: true

            Behavior on color { ColorAnimation { duration: 200 } }
        }

        // Audio level meter
        Loader {
            active: root.showAudioMeter
            Layout.preferredWidth: expanded ? 48 : 24
            Layout.preferredHeight: 8
            Layout.alignment: Qt.AlignVCenter

            Behavior on Layout.preferredWidth { NumberAnimation { duration: 200 } }

            sourceComponent: AudioLevelMeter {
                level: root.audioLevel
            }
        }
    }

    // Slight desaturation when not expanded
    layer.enabled: !expanded
    opacity: expanded ? 1.0 : 0.6
}
