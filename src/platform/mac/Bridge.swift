import Foundation
import ScreenCaptureKit

// C ABI entry points mirroring MacScreenCaptureBridge.h.
// Called from C++/Qt code; Swift types never cross the boundary.

@_cdecl("sc_picker_create")
public func sc_picker_create(_ ctx: UnsafeMutableRawPointer?,
                             _ onSelected: SourceSelectedCb?,
                             _ onCancelled: SelectionCancelledCb?,
                             _ onError: SelectionErrorCb?) -> OpaquePointer? {
    guard #available(macOS 12.3, *) else { return nil }
    guard let ctx = ctx,
          let onSelected = onSelected,
          let onCancelled = onCancelled,
          let onError = onError else { return nil }

    let picker = ScreenPicker(ctx: ctx,
                              onSelected: onSelected,
                              onCancelled: onCancelled,
                              onError: onError)
    return OpaquePointer(Unmanaged.passRetained(picker).toOpaque())
}

@_cdecl("sc_picker_destroy")
public func sc_picker_destroy(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 12.3, *) else { return }
    // Balance the passRetained from sc_picker_create.
    _ = Unmanaged<ScreenPicker>.fromOpaque(UnsafeRawPointer(handle)).takeRetainedValue()
}

@_cdecl("sc_picker_show_all")
public func sc_picker_show_all(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 12.3, *) else { return }
    let picker = Unmanaged<ScreenPicker>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    picker.showAll()
}

@_cdecl("sc_picker_show_display")
public func sc_picker_show_display(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 12.3, *) else { return }
    let picker = Unmanaged<ScreenPicker>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    picker.showDisplay()
}

@_cdecl("sc_picker_show_window")
public func sc_picker_show_window(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 12.3, *) else { return }
    let picker = Unmanaged<ScreenPicker>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    picker.showWindow()
}

@_cdecl("sc_filter_release")
public func sc_filter_release(_ filterHandle: UnsafeMutableRawPointer?) {
    guard let filterHandle = filterHandle else { return }
    guard #available(macOS 12.3, *) else { return }
    _ = Unmanaged<SCContentFilter>.fromOpaque(filterHandle).takeRetainedValue()
}
