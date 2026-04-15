import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

AbstractButton {
    id: root

    property string iconSource: ""

    leftPadding: 8
    rightPadding: 8
    topPadding: 4
    bottomPadding: 4

    background: Rectangle {
        radius: 6
        color: root.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
        Behavior on color { ColorAnimation { duration: 150 } }
    }

    contentItem: RowLayout {
        id: contentRow
        spacing: 4

        Image {
            visible: root.iconSource.length > 0
            source: root.iconSource
            sourceSize: Qt.size(14, 14)
            Layout.alignment: Qt.AlignVCenter
            opacity: root.hovered ? 0.9 : 0.5
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }

        Text {
            text: root.text
            font.pixelSize: 11
            font.weight: Font.Medium
            color: root.hovered ? Qt.rgba(1, 1, 1, 0.9) : Qt.rgba(1, 1, 1, 0.5)
            Layout.alignment: Qt.AlignVCenter
            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }
}
