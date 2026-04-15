import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

Item {
    id: root
    width: 600
    height: 160

    // Icon path helper
    readonly property string iconBase: "qrc:/ScreenCopy/resources/icons/"

    // Background bar
    Rectangle {
        id: hudBar
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 44
        radius: 8
        color: "transparent"
        implicitWidth: barLayout.implicitWidth + 20

        // Gradient background
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(28/255, 28/255, 36/255, 0.97) }
                GradientStop { position: 1.0; color: Qt.rgba(18/255, 18/255, 26/255, 0.96) }
            }
            border.width: 1
            border.color: Qt.rgba(80/255, 80/255, 120/255, 0.25)
        }

        RowLayout {
            id: barLayout
            anchors.centerIn: parent
            spacing: 6

            // Drag handle
            Item {
                Layout.preferredWidth: 24
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.centerIn: parent
                    source: iconBase + "drag-handle.svg"
                    width: 14; height: 14
                    sourceSize: Qt.size(48, 48)
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.3
                }

                MouseArea {
                    anchors.fill: parent
                    property point clickPos
                    onPressed: function(mouse) {
                        clickPos = Qt.point(mouse.x, mouse.y)
                    }
                    onPositionChanged: function(mouse) {
                        var delta = Qt.point(mouse.x - clickPos.x, mouse.y - clickPos.y)
                        var win = root.Window.window
                        if (win) {
                            win.x += delta.x
                            win.y += delta.y
                        }
                    }
                    cursorShape: Qt.OpenHandCursor
                }
            }

            // Source selector button
            HudGroupButton {
                enabled: !Recorder.recording
                opacity: enabled ? 1.0 : 0.5

                contentItem: RowLayout {
                    spacing: 4

                    Item {
                        Layout.preferredWidth: 18
                        Layout.preferredHeight: 18
                        Image {
                            anchors.centerIn: parent
                            width: 18; height: 18
                            source: iconBase + "monitor.svg"
                            sourceSize: Qt.size(48, 48)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            opacity: 0.8
                        }
                    }
                    Text {
                        text: Recorder.hasSelectedSource ? Recorder.selectedSource : qsTr("Select")
                        font.pixelSize: 11
                        color: Qt.rgba(1, 1, 1, 0.7)
                        elide: Text.ElideRight
                        Layout.maximumWidth: 72
                    }
                }

                onClicked: Recorder.openSourceSelector()
            }

            // Audio controls group
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: audioRow.implicitWidth + 8
                implicitHeight: 34
                radius: 8
                color: Qt.rgba(1, 1, 1, 0.05)

                RowLayout {
                    id: audioRow
                    anchors.centerIn: parent
                    spacing: 2

                    // System audio toggle
                    HudIconButton {
                        iconSource: iconBase + (Recorder.systemAudioEnabled ? "volume-up.svg" : "volume-off.svg")
                        active: Recorder.systemAudioEnabled
                        enabled: !Recorder.recording
                        onClicked: Recorder.systemAudioEnabled = !Recorder.systemAudioEnabled
                        ToolTip.text: Recorder.systemAudioEnabled ? qsTr("Disable system audio") : qsTr("Enable system audio")
                    }

                    // Microphone toggle
                    HudIconButton {
                        iconSource: iconBase + (Recorder.microphoneEnabled ? "mic.svg" : "mic-off.svg")
                        active: Recorder.microphoneEnabled
                        enabled: !Recorder.recording
                        onClicked: Recorder.microphoneEnabled = !Recorder.microphoneEnabled
                        ToolTip.text: Recorder.microphoneEnabled ? qsTr("Disable microphone") : qsTr("Enable microphone")
                    }

                    // Webcam toggle
                    HudIconButton {
                        iconSource: iconBase + (Recorder.webcamEnabled ? "videocam.svg" : "videocam-off.svg")
                        active: Recorder.webcamEnabled
                        onClicked: Recorder.webcamEnabled = !Recorder.webcamEnabled
                        ToolTip.text: Recorder.webcamEnabled ? qsTr("Disable webcam") : qsTr("Enable webcam")
                    }
                }
            }

            // Record / Stop button
            Rectangle {
                id: recordBtn
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: recordRow.implicitWidth + 16
                implicitHeight: 34
                radius: 8
                opacity: (!Recorder.recording && !Recorder.canRecord) ? 0.5 : 1.0

                color: {
                    if (Recorder.recording && !Recorder.paused)
                        return Qt.rgba(239/255, 68/255, 68/255, pulseAnim.pulseValue)
                    if (Recorder.recording && Recorder.paused)
                        return Qt.rgba(245/255, 158/255, 11/255, 0.1)
                    return Qt.rgba(1, 1, 1, 0.05)
                }

                Item {
                    id: pulseAnim
                    property real pulseValue: 0.1
                    visible: false

                    SequentialAnimation on pulseValue {
                        running: Recorder.recording && !Recorder.paused
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.2; duration: 750; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 0.05; duration: 750; easing.type: Easing.InOutQuad }
                    }
                }

                RowLayout {
                    id: recordRow
                    anchors.centerIn: parent
                    spacing: 6

                    // Record/stop icon — circle morphs to rounded square
                    Rectangle {
                        Layout.preferredWidth: Recorder.recording ? 14 : 16
                        Layout.preferredHeight: Recorder.recording ? 14 : 16
                        Layout.alignment: Qt.AlignVCenter
                        radius: Recorder.recording ? 2 : 7
                        color: {
                            if (Recorder.recording && Recorder.paused) return "#f59e0b"
                            if (Recorder.recording) return "#f87171"
                            if (Recorder.canRecord) return Qt.rgba(1, 1, 1, 0.8)
                            return Qt.rgba(1, 1, 1, 0.3)
                        }
                        Behavior on radius { NumberAnimation { duration: 150 } }
                    }

                    // Elapsed time
                    Text {
                        visible: Recorder.recording
                        text: Recorder.elapsedFormatted
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.family: "monospace"
                        color: Recorder.paused ? "#f59e0b" : "#f87171"
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: (Recorder.canRecord || Recorder.recording) ? Qt.PointingHandCursor : Qt.ArrowCursor
                    enabled: Recorder.canRecord || Recorder.recording
                    hoverEnabled: true
                    onClicked: {
                        if (Recorder.recording)
                            Recorder.stopRecording()
                        else
                            Recorder.startRecording()
                    }
                }

                HoverHandler { id: recordHover }
                scale: recordHover.hovered ? 1.08 : 1.0
                Behavior on scale { NumberAnimation { duration: 150 } }
            }

            // Pause / Resume (recording only)
            HudIconButton {
                visible: Recorder.recording
                iconSource: iconBase + (Recorder.paused ? "play.svg" : "pause.svg")
                onClicked: Recorder.togglePause()
                ToolTip.text: Recorder.paused ? qsTr("Resume recording") : qsTr("Pause recording")
            }

            // Restart (recording only)
            HudIconButton {
                visible: Recorder.recording
                iconSource: iconBase + "restart.svg"
                onClicked: Recorder.restartRecording()
                ToolTip.text: qsTr("Restart recording")
            }

            // Cancel (recording only)
            HudIconButton {
                visible: Recorder.recording
                iconSource: iconBase + "cancel.svg"
                onClicked: Recorder.cancelRecording()
                ToolTip.text: qsTr("Cancel recording")
            }

            // Open video file (not recording)
            HudIconButton {
                visible: !Recorder.recording
                iconSource: iconBase + "video-file.svg"
                onClicked: {} // TODO: open file picker
                ToolTip.text: qsTr("Open video file")
            }

            // Open project (not recording)
            HudIconButton {
                visible: !Recorder.recording
                iconSource: iconBase + "folder-open.svg"
                onClicked: {} // TODO: load project
                ToolTip.text: qsTr("Open project")
            }

            // Separator
            Rectangle {
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 1
                Layout.preferredHeight: 20
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            // Minimize
            HudIconButton {
                iconSource: iconBase + "minus.svg"
                small: true
                onClicked: {
                    var win = root.Window.window
                    if (win) win.showMinimized()
                }
                ToolTip.text: qsTr("Hide HUD")
            }

            // Close
            HudIconButton {
                iconSource: iconBase + "x-close.svg"
                small: true
                onClicked: Qt.quit()
                ToolTip.text: qsTr("Close app")
            }
        }
    }

    // Device selector panels (above HUD bar)
    DeviceSelector {
        id: micSelector
        visible: Recorder.microphoneEnabled && !Recorder.recording
        anchors.bottom: hudBar.top
        anchors.bottomMargin: 8
        anchors.horizontalCenter: hudBar.horizontalCenter
        label: qsTr("Microphone")
        showAudioMeter: true
        audioLevel: Recorder.audioLevel

        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    DeviceSelector {
        id: webcamSelector
        visible: Recorder.webcamEnabled && !Recorder.recording
        anchors.bottom: micSelector.visible ? micSelector.top : hudBar.top
        anchors.bottomMargin: 8
        anchors.horizontalCenter: hudBar.horizontalCenter
        label: qsTr("Webcam")
        showAudioMeter: false

        opacity: visible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }
}
