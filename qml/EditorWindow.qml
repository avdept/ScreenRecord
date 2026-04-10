import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ScreenCopy 1.0

ApplicationWindow {
    id: editorWindow
    width: 1200
    height: 800
    minimumWidth: 1024
    minimumHeight: 600
    visible: true
    title: "ScreenCopy — Editor"
    color: "#09090b"

    property string videoPath: ""

    // Keyboard shortcuts
    Shortcut {
        sequence: "S"
        onActivated: timelineEditor.activeTool = "select"
    }
    Shortcut {
        sequence: "T"
        onActivated: timelineEditor.activeTool = "trim"
    }

    onVideoPathChanged: {
        if (videoPath.length > 0) {
            videoPlayback.videoPlayer.source = videoPath;
        }
    }

    // Initialize TrimModel when video duration is known
    Connections {
        target: videoPlayback.videoPlayer
        function onDurationChanged() {
            if (videoPlayback.videoPlayer.duration > 0) {
                TrimModel.initialize(videoPlayback.videoPlayer.duration);
            }
        }
    }

    // Sync playback state to VideoPlayer
    Connections {
        target: Playback
        function onPlayingChanged() {
            if (Playback.playing && !videoPlayback.videoPlayer.playing)
                videoPlayback.videoPlayer.play();
            else if (!Playback.playing && videoPlayback.videoPlayer.playing)
                videoPlayback.videoPlayer.pause();
        }
    }

    // Sync trim segments to VideoPlayer for gapless playback
    Connections {
        target: TrimModel
        function onSegmentsChanged() {
            syncSegmentsToPlayer();
        }
    }

    function syncSegmentsToPlayer() {
        var segs = [];
        for (var i = 0; i < TrimModel.count; i++) {
            var startMs = TrimModel.data(TrimModel.index(i, 0), 257); // StartMsRole
            var endMs = TrimModel.data(TrimModel.index(i, 0), 258);   // EndMsRole
            segs.push({
                "startMs": startMs,
                "endMs": endMs
            });
        }
        videoPlayback.videoPlayer.setSegments(segs);
    }

    // Export finished notification
    Connections {
        target: Exporter
        function onExportFinished(path) {
            console.log("Export done:", path);
            Platform.revealInFolder(path);
        }
        function onExportError(msg) {
            console.error("Export error:", msg);
        }
    }

    // Top bar — h-10, bg with 80% opacity, backdrop blur
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 40
        color: Qt.rgba(9 / 255, 9 / 255, 11 / 255, 0.8)
        z: 50

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Qt.rgba(1, 1, 1, 0.05)
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            EditorTopBarButton {
                text: qsTr("New Recording")
                onClicked: {
                    editorWindow.visible = false;
                    // TODO: show HUD again
                }
            }

            EditorTopBarButton {
                iconSource: "qrc:/ScreenCopy/resources/icons/folder-open.svg"
                text: qsTr("Load Project")
            }

            EditorTopBarButton {
                text: qsTr("Save Project")
            }
        }
    }

    // Main content — p-5 gap-4 (20px padding, 16px gap)
    RowLayout {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 16

        // Left column — flex-[7], gap-3 (12px)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: 7
            spacing: 12

            // Video preview — rounded-2xl, border white/5, shadow-2xl
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 7
                radius: 4
                color: Qt.rgba(0, 0, 0, 0.4)   // bg-black/40
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.05)

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6    // mt-1.5 equivalent
                    spacing: 6

                    // Video area — always 16:9, centered in available space
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        VideoPlayback {
                            id: videoPlayback
                            anchors.centerIn: parent
                            width: {
                                var maxW = parent.width;
                                var maxH = parent.height;
                                var byWidth = maxW;
                                var byHeight = maxH * (16 / 9);
                                return Math.min(byWidth, byHeight);
                            }
                            height: width * (9 / 16)
                        }
                    }

                    // Playback controls — px-1 py-0.5
                    PlaybackControls {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        Layout.leftMargin: 4
                        Layout.rightMargin: 4
                        Layout.bottomMargin: 4

                        onSeekRequested: function (ms) {
                            videoPlayback.videoPlayer.seek(ms);
                        }
                        onPlayPauseRequested: {
                            videoPlayback.videoPlayer.togglePlayPause();
                        }
                    }
                }
            }

            // Timeline — rounded-2xl, border white/5, shadow-lg
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: 100
                radius: 4
                color: "#09090b"
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.05)

                TimelineEditor {
                    id: timelineEditor
                    anchors.fill: parent
                    anchors.margins: 1

                    onSeekRequested: function (ms) {
                        videoPlayback.videoPlayer.seek(ms);
                    }
                }
            }
        }

        // Right column — flex-[3], min-w-280, max-w-420
        // rounded-2xl, border white/5, shadow-xl
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 320
            Layout.minimumWidth: 280
            Layout.maximumWidth: 420
            radius: 4
            color: "#09090b"
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.05)

            SettingsPanel {
                anchors.fill: parent
                anchors.margins: 1

                onExportRequested: function (qualityIndex) {
                    Exporter.startExportWithDialog(editorWindow.videoPath, qualityIndex);
                }
            }
        }
    }
}
