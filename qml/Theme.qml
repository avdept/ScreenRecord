pragma Singleton
import QtQuick

QtObject {
    // Colors
    readonly property color background: "#1a1a2e"
    readonly property color surface: "#16213e"
    readonly property color surfaceLight: "#1f3460"
    readonly property color primary: "#6c6cff"
    readonly property color primaryHover: "#8484ff"
    readonly property color accent: "#e94560"
    readonly property color textPrimary: "#e0e0e0"
    readonly property color textSecondary: "#888888"
    readonly property color border: "#2a2a4a"
    readonly property color success: "#4caf50"
    readonly property color warning: "#ff9800"
    readonly property color error: "#f44336"

    // Typography
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeMedium: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 24
    readonly property int fontSizeHeading: 32

    // Spacing
    readonly property int spacingSmall: 4
    readonly property int spacingMedium: 8
    readonly property int spacingLarge: 16
    readonly property int spacingXLarge: 24

    // Border radius
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12
}
