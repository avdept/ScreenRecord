import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property real level: 0  // 0-100

    implicitWidth: 48
    implicitHeight: 8

    RowLayout {
        anchors.fill: parent
        spacing: 2

        Repeater {
            model: [
                { threshold: 10, height: 0.30 },
                { threshold: 25, height: 0.45 },
                { threshold: 45, height: 0.60 },
                { threshold: 65, height: 0.75 },
                { threshold: 85, height: 0.90 }
            ]

            Rectangle {
                required property var modelData
                required property int index

                Layout.fillWidth: true
                Layout.preferredHeight: parent.height * modelData.height
                Layout.alignment: Qt.AlignBottom
                radius: 1

                property bool isActive: root.level >= modelData.threshold

                color: {
                    if (!isActive) return "#334155"  // slate-700
                    if (modelData.threshold < 40) return "#10b981"  // emerald-500
                    if (modelData.threshold < 60) return "#22c55e"  // green-500
                    if (modelData.threshold < 80) return "#eab308"  // yellow-500
                    return "#ef4444"  // red-500
                }

                opacity: isActive ? 1.0 : 0.4

                Behavior on color { ColorAnimation { duration: 100 } }
                Behavior on opacity { NumberAnimation { duration: 100 } }
            }
        }
    }
}
