import Foundation
import ScreenCaptureKit
import CoreGraphics

/// Swift implementation of the ScreenCaptureKit source picker.
///
/// Holds C callback pointers and an opaque context from the C++ side.
/// All callbacks are invoked directly from SCK's internal dispatch queues;
/// the C++ trampoline is responsible for marshalling to the Qt main thread.
@available(macOS 12.3, *)
final class ScreenPicker: NSObject {
    // C callbacks + context, stored for the lifetime of the picker.
    private let ctx: UnsafeMutableRawPointer
    private let onSelected: SourceSelectedCb
    private let onCancelled: SelectionCancelledCb
    private let onError: SelectionErrorCb

    // The system picker is a shared singleton. We add ourselves as an observer
    // only while a picker invocation is active.
    private var isObserving: Bool = false

    init(ctx: UnsafeMutableRawPointer,
         onSelected: @escaping SourceSelectedCb,
         onCancelled: @escaping SelectionCancelledCb,
         onError: @escaping SelectionErrorCb) {
        self.ctx = ctx
        self.onSelected = onSelected
        self.onCancelled = onCancelled
        self.onError = onError
        super.init()
    }

    // MARK: - Entry points

    func showAll() {
        if #available(macOS 14, *) {
            presentNativePicker(modes: [.singleDisplay, .singleWindow])
        } else {
            enumerateAndSelectMainDisplay()
        }
    }

    func showDisplay() {
        // Direct enumeration is faster (no extra picker step) but needs
        // Screen Recording TCC. If the user hasn't granted it, fall back to
        // the native picker — the picker doesn't require TCC because the
        // user's selection in its UI serves as consent.
        if CGPreflightScreenCaptureAccess() {
            enumerateAndSelectMainDisplay()
            return
        }
        if #available(macOS 14, *) {
            presentNativePicker(modes: [.singleDisplay])
        } else {
            // Pre-14: no picker to fall back to; surface the permission request.
            CGRequestScreenCaptureAccess()
            enumerateAndSelectMainDisplay()
        }
    }

    func showWindow() {
        if #available(macOS 14, *) {
            presentNativePicker(modes: [.singleWindow])
        } else {
            enumerateAndSelectFrontmostWindow()
        }
    }

    // MARK: - Native picker (macOS 14+)

    @available(macOS 14, *)
    private func presentNativePicker(modes: SCContentSharingPickerMode) {
        let picker = SCContentSharingPicker.shared

        if picker.isActive {
            picker.isActive = false
            if isObserving {
                picker.remove(self)
                isObserving = false
            }
        }

        var config = SCContentSharingPickerConfiguration()
        config.allowedPickerModes = modes
        picker.defaultConfiguration = config

        picker.add(self)
        isObserving = true
        picker.isActive = true
        picker.present()
    }

    @available(macOS 14, *)
    private func dismissPicker() {
        let picker = SCContentSharingPicker.shared
        picker.isActive = false
        if isObserving {
            picker.remove(self)
            isObserving = false
        }
    }

    // MARK: - Fallback enumeration (macOS 12.3–13)

    private func enumerateAndSelectMainDisplay() {
        SCShareableContent.getExcludingDesktopWindows(false, onScreenWindowsOnly: false) { content, error in
            guard let content = content, error == nil, let display = content.displays.first else {
                let msg = error?.localizedDescription ?? "No displays found"
                msg.withCString { self.onError(self.ctx, $0) }
                return
            }

            let filter = SCContentFilter(display: display,
                                          excludingApplications: [],
                                          exceptingWindows: [])
            let filterHandle = Unmanaged.passRetained(filter).toOpaque()

            let sourceId = "display:\(display.displayID)"
            let sourceName = "Full Screen (\(display.width)×\(display.height))"

            sourceId.withCString { idPtr in
                sourceName.withCString { namePtr in
                    self.onSelected(self.ctx, idPtr, namePtr, filterHandle)
                }
            }
        }
    }

    private func enumerateAndSelectFrontmostWindow() {
        SCShareableContent.getExcludingDesktopWindows(true, onScreenWindowsOnly: true) { content, error in
            guard let content = content, error == nil, !content.windows.isEmpty else {
                let msg = error?.localizedDescription ?? "No windows found"
                msg.withCString { self.onError(self.ctx, $0) }
                return
            }

            // Prefer first on-screen window that has a title and an owning app.
            let target = content.windows.first(where: {
                $0.owningApplication != nil && !($0.title?.isEmpty ?? true)
            }) ?? content.windows[0]

            let filter = SCContentFilter(desktopIndependentWindow: target)
            let filterHandle = Unmanaged.passRetained(filter).toOpaque()

            let appName = target.owningApplication?.applicationName ?? "Unknown"
            let title = target.title ?? ""
            let sourceId = "window:\(target.windowID)"
            let sourceName = title.isEmpty ? appName : "\(appName) – \(title)"

            sourceId.withCString { idPtr in
                sourceName.withCString { namePtr in
                    self.onSelected(self.ctx, idPtr, namePtr, filterHandle)
                }
            }
        }
    }
}

// MARK: - SCContentSharingPickerObserver (macOS 14+)

@available(macOS 14, *)
extension ScreenPicker: SCContentSharingPickerObserver {
    func contentSharingPicker(_ picker: SCContentSharingPicker,
                              didUpdateWith filter: SCContentFilter,
                              for stream: SCStream?) {
        let filterHandle = Unmanaged.passRetained(filter).toOpaque()

        let rect = filter.contentRect
        let w = Int(rect.width)
        let h = Int(rect.height)
        let sourceId = "filter:\(w)x\(h)"
        let sourceName = "Selection (\(w)×\(h))"

        sourceId.withCString { idPtr in
            sourceName.withCString { namePtr in
                self.onSelected(self.ctx, idPtr, namePtr, filterHandle)
            }
        }

        dismissPicker()
    }

    func contentSharingPicker(_ picker: SCContentSharingPicker,
                              didCancelFor stream: SCStream?) {
        onCancelled(ctx)
        dismissPicker()
    }

    func contentSharingPickerStartDidFailWithError(_ error: any Error) {
        error.localizedDescription.withCString { self.onError(self.ctx, $0) }
    }
}
