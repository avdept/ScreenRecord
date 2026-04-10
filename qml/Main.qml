import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

ApplicationWindow {
    id: root
    width: 1200
    height: 800
    minimumWidth: 1024
    minimumHeight: 600
    visible: true
    title: "OpenScreen"
    color: "#1a1a2e"

    // HUD window instance
    HudWindow {
        id: hudWindow
        visible: false
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 16

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "OpenScreen"
            font.pixelSize: 32
            font.weight: Font.Bold
            color: "#e0e0e0"
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Qt6 Native Application"
            font.pixelSize: 16
            color: "#888"
        }

        Item { Layout.preferredHeight: 20 }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: "Show Recording HUD"
            font.pixelSize: 14
            onClicked: hudWindow.visible = true

            background: Rectangle {
                implicitWidth: 200
                implicitHeight: 44
                radius: 22
                color: parent.hovered ? "#8484ff" : "#6c6cff"
                Behavior on color { ColorAnimation { duration: 150 } }
            }
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Or run with --hud flag for standalone HUD mode"
            font.pixelSize: 12
            color: "#666"
        }

        Item { Layout.fillHeight: true }
    }
}
