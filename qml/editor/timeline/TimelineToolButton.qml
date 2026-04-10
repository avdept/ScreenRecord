import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: root

    property string iconSource: ""
    property string shortcutKey: ""
    property bool active: false

    readonly property color green: "#34B27B"

    implicitHeight: 26
    implicitWidth: contentRow.implicitWidth + 14
    leftPadding: 7
    rightPadding: 7

    background: Rectangle {
        radius: 6
        color: {
            if (root.active)
                return Qt.rgba(52/255, 178/255, 123/255, 0.2)
            if (root.hovered)
                return Qt.rgba(1, 1, 1, 0.08)
            return "transparent"
        }
        border.width: root.active ? 1 : 0
        border.color: Qt.rgba(52/255, 178/255, 123/255, 0.4)

        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 5

        Image {
            visible: root.iconSource.length > 0
            source: root.iconSource
            sourceSize: Qt.size(14, 14)
            Layout.alignment: Qt.AlignVCenter
            opacity: root.active ? 1.0 : (root.hovered ? 0.9 : 0.5)
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Text {
            text: root.text
            font.pixelSize: 11
            font.weight: Font.Medium
            color: "white"
            opacity: root.active ? 1.0 : (root.hovered ? 0.9 : 0.5)
            Layout.alignment: Qt.AlignVCenter
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        // Shortcut key badge
        Rectangle {
            visible: root.shortcutKey.length > 0
            Layout.preferredWidth: 16
            Layout.preferredHeight: 16
            Layout.alignment: Qt.AlignVCenter
            radius: 3
            color: Qt.rgba(1, 1, 1, 0.08)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.1)

            Text {
                anchors.centerIn: parent
                text: root.shortcutKey
                font.pixelSize: 9
                font.weight: Font.Medium
                color: Qt.rgba(1, 1, 1, 0.4)
            }
        }
    }

    scale: pressed ? 0.96 : 1.0
    Behavior on scale { NumberAnimation { duration: 100 } }
}
