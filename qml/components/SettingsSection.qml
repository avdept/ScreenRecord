import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    property string title: ""
    property color accentColor: Theme.green
    property bool expanded: true
    default property alias content: contentColumn.data

    Layout.fillWidth: true
    implicitHeight: sectionBox.implicitHeight
    implicitWidth: sectionBox.implicitWidth

    Rectangle {
        id: sectionBox
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: sectionCol.implicitHeight
        radius: 4
        color: Qt.rgba(1, 1, 1, 0.02)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.08)

        ColumnLayout {
            id: sectionCol
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0

            // Header
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 36

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 6

                    Text {
                        text: root.title
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        color: Theme.textLight
                        Layout.fillWidth: true
                    }

                    // Chevron
                    Image {
                        source: "qrc:/qt/qml/ScreenCopy/resources/icons/chevron-down.svg"
                        sourceSize: Qt.size(12, 12)
                        width: 12; height: 12
                        fillMode: Image.PreserveAspectFit
                        rotation: root.expanded ? 180 : 0
                        opacity: 0.4

                        Behavior on rotation { NumberAnimation { duration: 150 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.expanded = !root.expanded
                }
            }

            // Content wrapper with height animation
            Item {
                id: contentWrapper
                Layout.fillWidth: true
                Layout.preferredHeight: root.expanded ? contentColumn.implicitHeight + 14 : 0
                clip: true
                opacity: root.expanded ? 1.0 : 0.0

                Behavior on Layout.preferredHeight {
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 150 }
                }

                ColumnLayout {
                    id: contentColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 4
                    spacing: 8
                }
            }
        }
    }
}
