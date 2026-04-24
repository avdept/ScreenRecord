import AppKit

// Promote the HUD to status-bar level and make it visible across all Spaces,
// including fullscreen apps. Qt's WindowStaysOnTopHint maps to
// NSFloatingWindowLevel, which sits below fullscreen Spaces — users lose the
// HUD as soon as they record a fullscreen window, making it hard to stop.
@_cdecl("sc_hud_apply_window_rules")
public func sc_hud_apply_window_rules(_ viewPtr: UnsafeMutableRawPointer?) {
    guard let viewPtr = viewPtr else { return }
    let view = Unmanaged<NSView>.fromOpaque(viewPtr).takeUnretainedValue()
    guard let window = view.window else { return }

    // statusBar sits above menu bar and fullscreen apps but below system
    // screenSaver / popUpMenu levels — correct place for a recording HUD.
    window.level = .statusBar

    // Visible across all Spaces (including fullscreen) and doesn't fight the
    // fullscreen compositor by joining its auxiliary layer.
    window.collectionBehavior = [.canJoinAllSpaces,
                                  .fullScreenAuxiliary,
                                  .stationary]

    // Qt::Tool maps to NSPanel which hides on app deactivation by default —
    // that's what causes the HUD to vanish as soon as the user clicks the
    // window they're recording. Keep it visible regardless of app focus, and
    // don't steal key status unless genuinely clicked.
    if let panel = window as? NSPanel {
        panel.hidesOnDeactivate = false
        panel.becomesKeyOnlyIfNeeded = true
    }

    // Reposition above the Dock. NSScreen.visibleFrame already excludes the
    // Dock and menu bar, so visibleFrame.origin.y is the top of the Dock when
    // it's at the bottom (and the bottom of the screen area when the Dock is
    // on a side). Horizontally centered on whichever screen the window is on.
    if let screen = window.screen ?? NSScreen.main {
        let margin: CGFloat = 16
        let frame = window.frame
        let x = screen.frame.origin.x + (screen.frame.width - frame.width) / 2
        let y = screen.visibleFrame.origin.y + margin
        window.setFrameOrigin(NSPoint(x: x, y: y))
    }
}
