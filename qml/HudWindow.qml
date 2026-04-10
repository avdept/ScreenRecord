import QtQuick
import QtQuick.Controls
import ScreenCopy 1.0

ApplicationWindow {
    id: hudWindow
    width: 600
    height: 160
    visible: true
    color: "transparent"
    title: "screencopy-hud"

    flags: Qt.FramelessWindowHint
         | Qt.WindowStaysOnTopHint
         | Qt.Tool

    Component.onCompleted: {
        x = (Screen.width - width) / 2
        y = Screen.height - height - 40
        Platform.applyHudWindowRules(hudWindow)
    }

    // Editor window — created once, shown after recording
    EditorWindow {
        id: editorWindow
        visible: false
    }

    // When recording finishes, hide HUD and show editor
    Connections {
        target: Recorder
        function onRecordingFinished(videoPath) {
            editorWindow.videoPath = videoPath
            editorWindow.visible = true
            editorWindow.showMaximized()
            hudWindow.visible = false
        }
    }

    HudOverlay {
        anchors.fill: parent
    }
}
