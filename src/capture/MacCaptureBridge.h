#ifndef MAC_CAPTURE_BRIDGE_H
#define MAC_CAPTURE_BRIDGE_H

// C ABI between Swift (ScreenCaptureKit recording) and C++/Qt.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Opaque handle to a Swift-side ScreenRecorder instance.
typedef struct SCRecorder* SCRecorderHandle;

// POD capture options (mirrors RecordingOptions for the subset Swift needs).
typedef struct SCRecordingOptionsC {
    int32_t frame_rate;
    bool has_region;
    int32_t region_x;
    int32_t region_y;
    int32_t region_w;
    int32_t region_h;
} SCRecordingOptionsC;

// Callbacks — invoked from SCK dispatch queues. C++ trampoline marshals to Qt.
typedef void (*RecordingStartedCb)(void* ctx, const char* output_path);
typedef void (*RecordingStoppedCb)(void* ctx, const char* output_path);
typedef void (*RecordingErrorCb)(void* ctx, const char* message);

SCRecorderHandle sc_recorder_create(void* ctx,
                                    RecordingStartedCb on_started,
                                    RecordingStoppedCb on_stopped,
                                    RecordingErrorCb on_error);

void sc_recorder_destroy(SCRecorderHandle h);

// Returns true if recording started successfully (stream is spinning up).
// filter_handle ownership transfers to the recorder; it will release it.
// Pass NULL for filter_handle to auto-select the primary display.
bool sc_recorder_start(SCRecorderHandle h,
                       const char* output_path,
                       void* filter_handle,
                       SCRecordingOptionsC options);

void sc_recorder_stop(SCRecorderHandle h);
void sc_recorder_cancel(SCRecorderHandle h);

#ifdef __cplusplus
}
#endif

#endif // MAC_CAPTURE_BRIDGE_H
