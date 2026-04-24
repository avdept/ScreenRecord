import Foundation

@_cdecl("sc_recorder_create")
public func sc_recorder_create(_ ctx: UnsafeMutableRawPointer?,
                               _ onStarted: RecordingStartedCb?,
                               _ onStopped: RecordingStoppedCb?,
                               _ onError: RecordingErrorCb?) -> OpaquePointer? {
    guard #available(macOS 15, *) else { return nil }
    guard let ctx = ctx,
          let onStarted = onStarted,
          let onStopped = onStopped,
          let onError = onError else { return nil }

    let recorder = ScreenRecorder(ctx: ctx,
                                  onStarted: onStarted,
                                  onStopped: onStopped,
                                  onError: onError)
    return OpaquePointer(Unmanaged.passRetained(recorder).toOpaque())
}

@_cdecl("sc_recorder_destroy")
public func sc_recorder_destroy(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 15, *) else { return }
    _ = Unmanaged<ScreenRecorder>.fromOpaque(UnsafeRawPointer(handle)).takeRetainedValue()
}

@_cdecl("sc_recorder_start")
public func sc_recorder_start(_ handle: OpaquePointer?,
                              _ outputPath: UnsafePointer<CChar>?,
                              _ filterHandle: UnsafeMutableRawPointer?,
                              _ options: SCRecordingOptionsC) -> Bool {
    guard let handle = handle, let outputPath = outputPath else { return false }
    guard #available(macOS 15, *) else { return false }

    let recorder = Unmanaged<ScreenRecorder>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    let path = String(cString: outputPath)
    return recorder.start(path: path, filterHandle: filterHandle, options: options)
}

@_cdecl("sc_recorder_stop")
public func sc_recorder_stop(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 15, *) else { return }
    let recorder = Unmanaged<ScreenRecorder>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    recorder.stop()
}

@_cdecl("sc_recorder_cancel")
public func sc_recorder_cancel(_ handle: OpaquePointer?) {
    guard let handle = handle else { return }
    guard #available(macOS 15, *) else { return }
    let recorder = Unmanaged<ScreenRecorder>.fromOpaque(UnsafeRawPointer(handle)).takeUnretainedValue()
    recorder.cancel()
}
