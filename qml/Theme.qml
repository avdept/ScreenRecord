pragma Singleton
import QtQuick

QtObject {
    // Colors — surfaces
    readonly property color background: "#1a1a2e"
    readonly property color backgroundDark: "#09090b"
    readonly property color surface: "#16213e"
    readonly property color surfaceLight: "#1f3460"

    // Colors — accents
    readonly property color primary: "#6c6cff"
    readonly property color primaryHover: "#8484ff"
    readonly property color accent: "#e94560"
    readonly property color green: "#34B27B"
    readonly property color amber: "#f59e0b"
    readonly property color red: "#ef4444"
    readonly property color redLight: "#f87171"

    // Colors — text scale
    readonly property color textPrimary: "#e0e0e0"
    readonly property color textLight: "#e2e8f0"
    readonly property color textMuted: "#cbd5e1"
    readonly property color textSlate: "#94a3b8"
    readonly property color textSecondary: "#888888"
    readonly property color textDim: "#64748b"
    readonly property color textInactive: "#334155"

    // Colors — semantic
    readonly property color border: "#2a2a4a"
    readonly property color success: "#4caf50"
    readonly property color warning: "#ff9800"
    readonly property color error: "#f44336"

    // Colors — base
    readonly property color white: "#ffffff"
    readonly property color black: "#000000"

    // Colors — HUD
    readonly property color hudSurface: "#F81C1C24"
    readonly property color hudSurfaceAlt: "#F512121A"
    readonly property color hudBorder: "#40505078"

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
