#ifndef MAC_SCREEN_CAPTURE_BRIDGE_H
#define MAC_SCREEN_CAPTURE_BRIDGE_H

// C ABI between Swift (ScreenCaptureKit source selection) and C++/Qt.
// Must be plain C — included by both .cpp (C++) and .swift (via bridging header).

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// Opaque handle to a Swift-side ScreenPicker instance.
typedef struct SCPicker* SCPickerHandle;

// Callback prototypes. All called directly from SCK dispatch queues.
// The C++ trampoline is responsible for marshalling to the Qt thread.
// Strings are UTF-8 and only valid for the duration of the call.
typedef void (*SourceSelectedCb)(void* ctx,
                                 const char* source_id,
                                 const char* source_name,
                                 void* filter_handle);
typedef void (*SelectionCancelledCb)(void* ctx);
typedef void (*SelectionErrorCb)(void* ctx, const char* message);

// Create a picker. `ctx` is opaque user data passed back to every callback.
// Returns NULL if ScreenCaptureKit is unavailable on this OS.
SCPickerHandle sc_picker_create(void* ctx,
                                SourceSelectedCb on_selected,
                                SelectionCancelledCb on_cancelled,
                                SelectionErrorCb on_error);

void sc_picker_destroy(SCPickerHandle h);

// Show the native picker (macOS 14+) or fall back to enumeration.
void sc_picker_show_all(SCPickerHandle h);     // both display + window
void sc_picker_show_display(SCPickerHandle h); // display only (auto-picks primary on fallback)
void sc_picker_show_window(SCPickerHandle h);  // window only

// Release a filter handle previously delivered via SourceSelectedCb.
// Safe to call with NULL.
void sc_filter_release(void* filter_handle);

// Promote the HUD window to always-on-top across all Spaces / fullscreen apps.
// `ns_view` is the QWindow::winId() cast to void* — on macOS that's an
// NSView* whose .window we mutate. Safe to call with NULL.
void sc_hud_apply_window_rules(void* ns_view);

#ifdef __cplusplus
}
#endif

#endif // MAC_SCREEN_CAPTURE_BRIDGE_H
