import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

Item {
    id: root
    clip: true

    readonly property color green: Theme.green
    property int bgTab: 0  // 0=Image, 1=Color

    property int selectedQuality: 1  // 0=Medium, 1=Good, 2=Source
    signal exportRequested(int qualityIndex)

    Flickable {
        anchors.fill: parent
        contentHeight: settingsCol.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: settingsCol
            width: parent.width
            spacing: 12

            // --- SELECTED SEGMENT SETTINGS ---
            Item {
                visible: TrimModel.selectedIndex >= 0
                Layout.fillWidth: true
                implicitHeight: segPropsCol.implicitHeight

                ColumnLayout {
                    id: segPropsCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    spacing: 10

                    // Header with title and close button
                    RowLayout {
                        Layout.fillWidth: true

                        Text {
                            text: qsTr("Segment Properties")
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            color: Theme.textLight
                            Layout.fillWidth: true
                        }

                        // Close button
                        Rectangle {
                            width: 20
                            height: 20
                            radius: 4
                            color: closeHover.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                            Image {
                                anchors.centerIn: parent
                                width: 12; height: 12
                                source: "qrc:/qt/qml/ScreenCopy/resources/icons/x-close.svg"
                                sourceSize: Qt.size(48, 48)
                                fillMode: Image.PreserveAspectFit
                                opacity: closeHover.hovered ? 0.9 : 0.4
                            }

                            HoverHandler { id: closeHover }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: TrimModel.clearSelection()
                            }
                        }
                    }

                    // Speed section
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: speedCol.implicitHeight + 20
                        radius: 4
                        color: Qt.rgba(1, 1, 1, 0.02)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.08)

                        ColumnLayout {
                            id: speedCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 8

                            Text {
                                text: qsTr("Speed")
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                color: Theme.textSlate
                            }

                    GridLayout {
                        columns: 3
                        columnSpacing: 4
                        rowSpacing: 4
                        Layout.fillWidth: true

                        Repeater {
                            model: [0.5, 1.0, 1.5, 2.0, 3.0, 4.0]

                            Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 26
                                radius: 4
                                border.width: 1
                                property bool isActive: Math.abs(TrimModel.selectedSpeed - modelData) < 0.01
                                color: isActive ? root.green : Qt.rgba(1, 1, 1, 0.05)
                                border.color: isActive ? root.green : Qt.rgba(1, 1, 1, 0.08)

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData + "x"
                                    font.pixelSize: 10
                                    font.weight: Font.Medium
                                    color: isActive ? Theme.white : Theme.textSlate
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: TrimModel.selectedSpeed = modelData
                                }
                            }
                        }
                    }
                        }
                    }
                }
            }

            // --- SELECTED EFFECT REGION SETTINGS (shown when an effect region is selected) ---
            SettingsSection {
                visible: EffectRegions.selectedId >= 0 && TrimModel.selectedIndex < 0
                title: {
                    var t = EffectRegions.selectedType
                    if (t === 0) return qsTr("Zoom Settings")
                    if (t === 1) return qsTr("Speed Settings")
                    if (t === 2) return qsTr("Annotation Settings")
                    return qsTr("Region Settings")
                }
                accentColor: {
                    var t = EffectRegions.selectedType
                    if (t === 0) return root.green
                    if (t === 1) return Theme.amber
                    return "#b4a046"
                }

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    // Zoom region controls
                    Loader {
                        active: EffectRegions.selectedType === 0
                        visible: active
                        Layout.fillWidth: true
                        sourceComponent: ColumnLayout {
                            spacing: 8

                            Text {
                                text: qsTr("Depth")
                                font.pixelSize: 10
                                color: Qt.rgba(1, 1, 1, 0.5)
                            }

                            GridLayout {
                                columns: 6
                                columnSpacing: 6
                                rowSpacing: 6
                                Layout.fillWidth: true

                                Repeater {
                                    model: [1, 2, 3, 4, 5, 6]
                                    Rectangle {
                                        required property int modelData
                                        property int currentDepth: EffectRegions.selectedSettings.depth || 2
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 32
                                        radius: 8
                                        border.width: 1
                                        border.color: modelData === currentDepth ? root.green : Qt.rgba(1, 1, 1, 0.05)
                                        color: modelData === currentDepth ? root.green : Qt.rgba(1, 1, 1, 0.05)

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData.toString()
                                            font.pixelSize: 11
                                            font.weight: Font.Medium
                                            color: modelData === parent.currentDepth ? Theme.white : Theme.textSlate
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: EffectRegions.updateSelectedSetting("depth", modelData)
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Speed region controls
                    Loader {
                        active: EffectRegions.selectedType === 1
                        visible: active
                        Layout.fillWidth: true
                        sourceComponent: ColumnLayout {
                            spacing: 8
                            SettingsSlider {
                                label: qsTr("Speed")
                                value: EffectRegions.selectedSettings.speed || 1.0
                                onMoved: function(v) { EffectRegions.updateSelectedSetting("speed", v) }
                                from: 0.5; to: 4.0; stepSize: 0.25
                                accentColor: Theme.amber
                            }
                        }
                    }

                    // Deselect button
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        radius: 6
                        color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.08)

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Done")
                            font.pixelSize: 10
                            color: Qt.rgba(1, 1, 1, 0.5)
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: EffectRegions.selectedId = -1
                        }
                    }
                }
            }

            // --- EFFECTS SECTION (global, hidden when region selected) ---
            SettingsSection {
                visible: EffectRegions.selectedId < 0 && TrimModel.selectedIndex < 0
                title: qsTr("Effects")
                accentColor: root.green

                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    // Shadow
                    SettingsSlider {
                        label: qsTr("Shadow")
                        value: GlobalSettings.shadowIntensity
                        onMoved: function(v) { GlobalSettings.shadowIntensity = v }
                        from: 0; to: 1; stepSize: 0.01
                        accentColor: root.green
                    }

                    // Border radius
                    SettingsSlider {
                        label: qsTr("Border Radius")
                        value: GlobalSettings.borderRadius
                        onMoved: function(v) { GlobalSettings.borderRadius = v }
                        from: 0; to: 16; stepSize: 0.5
                        accentColor: root.green
                    }

                    // Padding
                    SettingsSlider {
                        label: qsTr("Padding")
                        value: GlobalSettings.padding
                        onMoved: function(v) { GlobalSettings.padding = v }
                        from: 0; to: 100; stepSize: 1
                        accentColor: root.green
                    }
                }
            }

            // --- BACKGROUND SECTION (global) ---
            SettingsSection {
                visible: EffectRegions.selectedId < 0 && TrimModel.selectedIndex < 0
                title: qsTr("Background")
                accentColor: root.green

                ColumnLayout {
                    spacing: 8
                    Layout.fillWidth: true

                    // Tab bar: Color / Image
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.05)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 2
                            spacing: 2

                            Repeater {
                                model: [qsTr("Image"), qsTr("Color")]

                                Rectangle {
                                    required property string modelData
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: root.bgTab === index ? root.green : "transparent"

                                    Behavior on color { ColorAnimation { duration: 150 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.pixelSize: 10
                                        color: root.bgTab === index ? Theme.white : Theme.textSlate
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.bgTab = index
                                    }
                                }
                            }
                        }
                    }

                    // Color grid — 16 generic colors
                    GridLayout {
                        visible: root.bgTab === 1
                        columns: 8
                        columnSpacing: 6
                        rowSpacing: 6
                        Layout.fillWidth: true

                        Repeater {
                            model: [
                                "#1a1a2e", "#0f0f0f", "#1e293b", "#1e3a5f",
                                "#14532d", "#3f6212", "#713f12", "#7f1d1d",
                                "#581c87", "#4c1d95", "#be185d", "#e11d48",
                                "#0ea5e9", "#10b981", "#f59e0b", "#ffffff"
                            ]

                            Rectangle {
                                required property string modelData
                                required property int index
                                Layout.preferredWidth: 30
                                Layout.preferredHeight: 30
                                radius: 6
                                color: modelData
                                border.width: 2
                                border.color: GlobalSettings.wallpaper === modelData ? root.green : Qt.rgba(1, 1, 1, 0.1)

                                // Checkerboard for white
                                Rectangle {
                                    visible: modelData === "#ffffff"
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    radius: 4
                                    color: "#f8f8f8"
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: GlobalSettings.wallpaper = modelData
                                }

                                // Selection ring
                                Rectangle {
                                    visible: GlobalSettings.wallpaper === modelData
                                    anchors.fill: parent
                                    anchors.margins: -2
                                    radius: 8
                                    color: "transparent"
                                    border.width: 1
                                    border.color: Qt.rgba(root.green.r, root.green.g, root.green.b, 0.3)
                                }
                            }
                        }
                    }

                    // Image grid — 18 wallpapers
                    GridLayout {
                        visible: root.bgTab === 0
                        columns: 6
                        columnSpacing: 6
                        rowSpacing: 6
                        Layout.fillWidth: true

                        Repeater {
                            model: 18

                            Rectangle {
                                required property int index
                                property string wpName: "wallpaper" + (index + 1)
                                property bool isSelected: GlobalSettings.wallpaper === wpName

                                Layout.preferredWidth: 40
                                Layout.preferredHeight: 40
                                radius: 6
                                color: Qt.rgba(1, 1, 1, 0.05)
                                border.width: 2
                                border.color: isSelected ? root.green : Qt.rgba(1, 1, 1, 0.1)
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    source: "qrc:/qt/qml/ScreenCopy/resources/wallpapers/" + wpName + ".jpg"
                                    fillMode: Image.PreserveAspectCrop
                                    smooth: true
                                    sourceSize: Qt.size(80, 80) // small thumbnail
                                }

                                // Hover brighten
                                Rectangle {
                                    anchors.fill: parent
                                    radius: parent.radius
                                    color: "transparent"
                                    border.width: 2
                                    border.color: hoverArea.containsMouse && !isSelected
                                                  ? Qt.rgba(root.green.r, root.green.g, root.green.b, 0.4)
                                                  : "transparent"
                                }

                                MouseArea {
                                    id: hoverArea
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    hoverEnabled: true
                                    onClicked: GlobalSettings.wallpaper = wpName
                                }

                                // Selection ring
                                Rectangle {
                                    visible: isSelected
                                    anchors.fill: parent
                                    anchors.margins: -2
                                    radius: 8
                                    color: "transparent"
                                    border.width: 1
                                    border.color: Qt.rgba(root.green.r, root.green.g, root.green.b, 0.3)
                                }
                            }
                        }
                    }
                }
            }

            // --- EXPORT SECTION (global) ---
            SettingsSection {
                visible: EffectRegions.selectedId < 0 && TrimModel.selectedIndex < 0
                title: qsTr("Export")
                accentColor: root.green

                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true

                    // Quality: Medium / Good / Source
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 28
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.05)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 2
                            spacing: 2

                            Repeater {
                                model: [qsTr("Medium"), qsTr("Good"), qsTr("Source")]

                                Rectangle {
                                    required property string modelData
                                    required property int index
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 6
                                    color: root.selectedQuality === index ? Theme.white : "transparent"

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.pixelSize: 10
                                        color: root.selectedQuality === index ? Theme.black : Theme.textSlate
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.selectedQuality = index
                                    }
                                }
                            }
                        }
                    }

                    // Progress bar (visible during export)
                    Rectangle {
                        visible: Exporter.exporting
                        Layout.fillWidth: true
                        Layout.preferredHeight: 6
                        radius: 3
                        color: Qt.rgba(1, 1, 1, 0.1)

                        Rectangle {
                            width: parent.width * Exporter.progress
                            height: parent.height
                            radius: parent.radius
                            color: root.green

                            Behavior on width { NumberAnimation { duration: 100 } }
                        }
                    }

                    // Status text during export
                    Text {
                        visible: Exporter.exporting
                        text: Exporter.statusText
                        font.pixelSize: 10
                        color: Theme.textSlate
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    // Export / Cancel button
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        radius: 12
                        color: Exporter.exporting ? Theme.red : root.green

                        Behavior on color { ColorAnimation { duration: 200 } }

                        Text {
                            anchors.centerIn: parent
                            text: Exporter.exporting ? qsTr("Cancel Export") : qsTr("Export Video")
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: Theme.white
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            onClicked: {
                                if (Exporter.exporting) {
                                    Exporter.cancelExport()
                                } else {
                                    root.exportRequested(root.selectedQuality)
                                }
                            }
                        }

                        HoverHandler { id: exportHover }
                        scale: exportHover.hovered ? 1.02 : 1.0
                        Behavior on scale { NumberAnimation { duration: 200 } }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.preferredHeight: 16 }
        }
    }
}
