import Foundation
import ScreenCaptureKit
import AVFoundation
import AppKit
import CoreMedia
import VideoToolbox

/// Swift-side recording engine using SCStream → SCStreamOutput → AVAssetWriter.
///
/// We can't use SCRecordingOutput because it doesn't actually support
/// `.hevcWithAlpha` at runtime (-3812 SCStreamErrorInvalidParameter).
/// Manual AVAssetWriter lets us preserve window alpha (rounded corners) in the
/// recorded .mov file, giving the editor real transparency to work with later
/// (shadows, backgrounds, compositing, etc.).
///
/// Pattern adapted from Nonstrict's ScreenCaptureKit-Recording-example.
@available(macOS 15, *)
final class ScreenRecorder: NSObject {
    private let ctx: UnsafeMutableRawPointer
    private let onStarted: RecordingStartedCb
    private let onStopped: RecordingStoppedCb
    private let onError: RecordingErrorCb

    // Pipeline
    private var stream: SCStream?
    private var assetWriter: AVAssetWriter?
    private var videoInput: AVAssetWriterInput?
    private var streamOutput: StreamOutput?
    private var outputPath: String = ""

    // SCK sample callbacks run on this dedicated queue.
    private let sampleQueue = DispatchQueue(label: "ScreenRecorder.SampleQueue")

    init(ctx: UnsafeMutableRawPointer,
         onStarted: @escaping RecordingStartedCb,
         onStopped: @escaping RecordingStoppedCb,
         onError: @escaping RecordingErrorCb) {
        self.ctx = ctx
        self.onStarted = onStarted
        self.onStopped = onStopped
        self.onError = onError
        super.init()
    }

    // MARK: - Start

    /// filterHandle — an `Unmanaged<SCContentFilter>` raw pointer (retained).
    /// If nil, we enumerate and pick the primary display.
    func start(path: String,
               filterHandle: UnsafeMutableRawPointer?,
               options: SCRecordingOptionsC) -> Bool {
        self.outputPath = path

        if let fh = filterHandle {
            let filter = Unmanaged<SCContentFilter>.fromOpaque(fh).takeRetainedValue()
            startWithFilter(filter, options: options, path: path)
            return true
        }

        // Fallback path: main display
        SCShareableContent.getExcludingDesktopWindows(false, onScreenWindowsOnly: false) { [weak self] content, error in
            guard let self = self else { return }
            if let error = error {
                self.openScreenCapturePrivacySettings()
                self.emitError(error.localizedDescription)
                return
            }
            guard let display = content?.displays.first else {
                self.emitError("No displays available")
                return
            }
            let filter = SCContentFilter(display: display,
                                          excludingApplications: [],
                                          exceptingWindows: [])
            self.startWithFilter(filter, options: options, path: path)
        }
        return true
    }

    private func startWithFilter(_ filter: SCContentFilter,
                                 options: SCRecordingOptionsC,
                                 path: String) {
        // ── 1. Determine output dimensions (multiple of 16 for HW encoder). ──
        let (width, height) = computeDimensions(filter: filter, options: options)
        guard width > 0, height > 0 else {
            emitError("Could not determine output dimensions")
            return
        }

        // ── 2. Set up AVAssetWriter. ──
        do {
            let url = URL(fileURLWithPath: path)
            let writer = try AVAssetWriter(outputURL: url, fileType: .mov)

            // Minimal settings matching Apple's "Using HEVC video with alpha" doc.
            // Adding MaxKeyFrameInterval / ColorProperties empirically causes
            // AVAssetWriter to silently demote to plain HEVC on some systems.
            let outputSettings: [String: Any] = [
                AVVideoCodecKey: AVVideoCodecType.hevcWithAlpha,
                AVVideoWidthKey: width,
                AVVideoHeightKey: height,
                AVVideoCompressionPropertiesKey: [
                    kVTCompressionPropertyKey_Quality as String: 0.75,
                    kVTCompressionPropertyKey_TargetQualityForAlpha as String: 0.9,
                ],
            ]

            let input = AVAssetWriterInput(mediaType: .video, outputSettings: outputSettings)
            input.expectsMediaDataInRealTime = true

            guard writer.canAdd(input) else {
                emitError("AVAssetWriter cannot add video input")
                return
            }
            writer.add(input)

            guard writer.startWriting() else {
                let msg = writer.error?.localizedDescription ?? "AVAssetWriter startWriting failed"
                emitError(msg)
                return
            }

            self.assetWriter = writer
            self.videoInput = input
        } catch {
            emitError("AVAssetWriter init failed: \(error.localizedDescription)")
            return
        }

        // ── 3. Build SCStream with our SCStreamOutput delegate. ──
        let config = buildStreamConfig(width: width, height: height, options: options)
        let stream = SCStream(filter: filter, configuration: config, delegate: self)
        let streamOutput = StreamOutput(videoInput: videoInput!)

        do {
            try stream.addStreamOutput(streamOutput, type: .screen, sampleHandlerQueue: sampleQueue)
        } catch {
            emitError("addStreamOutput failed: \(error.localizedDescription)")
            return
        }

        self.stream = stream
        self.streamOutput = streamOutput

        // ── 4. Start capture. Only then start the writer session. ──
        stream.startCapture { [weak self] error in
            guard let self = self else { return }
            if let error = error as NSError? {
                self.cleanup()
                self.emitError("startCapture failed: \(error.localizedDescription) [\(error.domain) \(error.code)]")
                return
            }
            self.assetWriter?.startSession(atSourceTime: .zero)
            self.streamOutput?.sessionStarted = true
            self.emitStarted(self.outputPath)
        }
    }

    // MARK: - Stop

    func stop() {
        guard let stream = self.stream else { return }
        stream.stopCapture { [weak self] _ in
            guard let self = self else { return }
            self.finalizeWriter()
        }
    }

    func cancel() {
        let path = outputPath
        stream?.stopCapture { _ in }
        assetWriter?.cancelWriting()
        cleanup()
        if !path.isEmpty {
            try? FileManager.default.removeItem(atPath: path)
        }
    }

    private func finalizeWriter() {
        guard let writer = self.assetWriter,
              let input = self.videoInput,
              let streamOutput = self.streamOutput else {
            cleanup()
            return
        }

        // Static-content tail trick: duplicate last frame at current time so
        // stationary recordings reach their true wall-clock duration. Without
        // this, a recording where the screen doesn't change ends at whatever
        // timestamp the last delivered frame had.
        let tailTime: CMTime
        if let last = streamOutput.lastSampleBuffer {
            let now = CMTime(seconds: ProcessInfo.processInfo.systemUptime,
                             preferredTimescale: 600)
            let pts = CMTimeSubtract(now, streamOutput.firstSampleTime)
            if let retimed = try? CMSampleBuffer(copying: last,
                                                 withNewTiming: [CMSampleTimingInfo(
                                                    duration: last.duration,
                                                    presentationTimeStamp: pts,
                                                    decodeTimeStamp: last.decodeTimeStamp)]) {
                if input.isReadyForMoreMediaData {
                    input.append(retimed)
                }
            }
            tailTime = pts
        } else {
            tailTime = .zero
        }

        writer.endSession(atSourceTime: tailTime)
        input.markAsFinished()

        let path = self.outputPath
        writer.finishWriting { [weak self] in
            guard let self = self else { return }
            if writer.status == .failed {
                self.emitError(writer.error?.localizedDescription ?? "AVAssetWriter finishWriting failed")
            } else {
                self.emitStopped(path)
            }
            self.cleanup()
        }
    }

    private func cleanup() {
        stream = nil
        assetWriter = nil
        videoInput = nil
        streamOutput = nil
        outputPath = ""
    }

    // MARK: - Config helpers

    private func computeDimensions(filter: SCContentFilter, options: SCRecordingOptionsC) -> (Int, Int) {
        func roundDown16(_ v: Int) -> Int { (v / 16) * 16 }

        if options.has_region {
            return (roundDown16(Int(options.region_w) * 2),
                    roundDown16(Int(options.region_h) * 2))
        }
        let rect = filter.contentRect
        let scale = filter.pointPixelScale > 0 ? filter.pointPixelScale : 2.0
        return (roundDown16(Int(rect.width * CGFloat(scale))),
                roundDown16(Int(rect.height * CGFloat(scale))))
    }

    private func buildStreamConfig(width: Int, height: Int, options: SCRecordingOptionsC) -> SCStreamConfiguration {
        let config = SCStreamConfiguration()
        config.width = width
        config.height = height
        config.minimumFrameInterval = CMTimeMake(value: 1, timescale: options.frame_rate)
        config.showsCursor = true
        config.capturesAudio = false

        // Alpha pipeline: BGRA + clear background so window rounded corners come
        // through as transparent pixels, encoded by .hevcWithAlpha above.
        config.pixelFormat = kCVPixelFormatType_32BGRA
        if #available(macOS 13, *) {
            config.backgroundColor = CGColor.clear
        }

        // Recommended ≥ 4, 6 keeps the pipeline smooth enough to retain a
        // lastSampleBuffer reference for the stop-tail trick.
        config.queueDepth = 6

        config.scalesToFit = true
        if options.has_region {
            config.sourceRect = CGRect(x: CGFloat(options.region_x),
                                       y: CGFloat(options.region_y),
                                       width: CGFloat(options.region_w),
                                       height: CGFloat(options.region_h))
        }
        return config
    }

    private func openScreenCapturePrivacySettings() {
        if let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture") {
            DispatchQueue.main.async { NSWorkspace.shared.open(url) }
        }
    }

    // MARK: - Callback helpers

    private func emitStarted(_ path: String) {
        path.withCString { self.onStarted(self.ctx, $0) }
    }

    private func emitStopped(_ path: String) {
        path.withCString { self.onStopped(self.ctx, $0) }
    }

    private func emitError(_ msg: String) {
        msg.withCString { self.onError(self.ctx, $0) }
    }
}

// MARK: - SCStreamDelegate

@available(macOS 15, *)
extension ScreenRecorder: SCStreamDelegate {
    func stream(_ stream: SCStream, didStopWithError error: any Error) {
        let ns = error as NSError
        emitError("stream stopped: \(ns.localizedDescription) [\(ns.domain) \(ns.code)]")
    }
}

// MARK: - SCStreamOutput (nested class)
//
// Lives separately from ScreenRecorder so it can mutate its own state from
// SCK's sample queue without capturing self in a mutable way.

@available(macOS 15, *)
private final class StreamOutput: NSObject, SCStreamOutput {
    let videoInput: AVAssetWriterInput

    var sessionStarted: Bool = false
    var firstSampleTime: CMTime = .zero
    var lastSampleBuffer: CMSampleBuffer?

    init(videoInput: AVAssetWriterInput) {
        self.videoInput = videoInput
        super.init()
    }

    func stream(_ stream: SCStream,
                didOutputSampleBuffer sampleBuffer: CMSampleBuffer,
                of type: SCStreamOutputType) {
        guard type == .screen else { return }
        guard sessionStarted, sampleBuffer.isValid else { return }

        // Drop incomplete frames — SCK delivers "idle" frames when content
        // doesn't change, and their IOSurface isn't necessarily valid.
        guard let attachmentsArray = CMSampleBufferGetSampleAttachmentsArray(
                    sampleBuffer, createIfNecessary: false) as? [[SCStreamFrameInfo: Any]],
              let attachments = attachmentsArray.first,
              let rawStatus = attachments[.status] as? Int,
              SCFrameStatus(rawValue: rawStatus) == .complete else {
            return
        }

        guard videoInput.isReadyForMoreMediaData else {
            // Writer backed up — drop this frame rather than stalling the queue.
            return
        }

        if firstSampleTime == .zero {
            firstSampleTime = sampleBuffer.presentationTimeStamp
        }

        let pts = CMTimeSubtract(sampleBuffer.presentationTimeStamp, firstSampleTime)
        lastSampleBuffer = sampleBuffer

        let timing = CMSampleTimingInfo(duration: sampleBuffer.duration,
                                        presentationTimeStamp: pts,
                                        decodeTimeStamp: sampleBuffer.decodeTimeStamp)
        if let retimed = try? CMSampleBuffer(copying: sampleBuffer, withNewTiming: [timing]) {
            videoInput.append(retimed)
            lastSampleBuffer = retimed
        }
    }
}
