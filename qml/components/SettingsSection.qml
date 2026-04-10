import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Collapsible settings section matching Electron's Accordion style
Item {
    id: root

    property string title: ""
    property color accentColor: "#34B27B"
    property bool expanded: true
    default property alias content: contentColumn.data

    Layout.fillWidth: true
    implicitHeight: sectionCol.implicitHeight
    implicitWidth: sectionCol.implicitWidth

    ColumnLayout {
        id: sectionCol
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            radius: expanded ? 12 : 12
            color: Qt.rgba(1, 1, 1, 0.02)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                // Accent dot
                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: root.accentColor
                }

                Text {
                    text: root.title
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: "#e2e8f0"  // slate-200
                    Layout.fillWidth: true
                }

                // Chevron
                Text {
                    text: root.expanded ? "\u25B4" : "\u25BE"
                    font.pixelSize: 10
                    color: Qt.rgba(1, 1, 1, 0.3)
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.expanded = !root.expanded
            }
        }

        // Content
        ColumnLayout {
            id: contentColumn
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.topMargin: 8
            Layout.bottomMargin: 12
            visible: root.expanded
            spacing: 8
        }
    }
}
