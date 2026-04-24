#include <QtTest/QtTest>
#include <QSignalSpy>

#include "MacCaptureBackend.h"
#include "MacPlatform.h"
#include "MacScreenCaptureBridge.h"
#include "MacCaptureBridge.h"

using namespace screencopy;

// Smoke tests for the macOS capture/platform classes. These verify the
// C ABI bridge wiring, object construction, and state-machine guards.
//
// They do NOT attempt real screen recording — CI runners don't have TCC
// (Screen Recording permission) and we'd need a real display anyway. Real
// recording behavior is exercised manually in dev.
class TstMacCapture : public QObject
{
    Q_OBJECT

private slots:
    // ── MacPlatform ────────────────────────────────────────────

    void platformConstructs()
    {
        MacPlatform platform;
        QCOMPARE(platform.platformName(), QStringLiteral("macos"));
        QVERIFY(!platform.recordingsDir().isEmpty());
    }

    void platformTakeLastFilterIsNullByDefault()
    {
        MacPlatform platform;
        QVERIFY(platform.takeLastFilter() == nullptr);
    }

    // ── MacCaptureBackend ──────────────────────────────────────

    void backendConstructs()
    {
        MacPlatform platform;
        MacCaptureBackend backend(&platform);

        QCOMPARE(backend.preferredExtension(), QStringLiteral("mov"));
        QCOMPARE(backend.state(), RecordingState::Idle);
        // isAvailable depends on the runner's macOS version (>=15 for
        // SCRecordingOutput-era APIs). Just verify it returns without crashing.
        (void)backend.isAvailable();
    }

    void backendCancelOnIdleIsNoOp()
    {
        MacPlatform platform;
        MacCaptureBackend backend(&platform);

        // Cancel on a fresh backend should not throw or change state.
        backend.cancelRecording();
        QCOMPARE(backend.state(), RecordingState::Idle);
    }

    void backendStopOnIdleIsNoOp()
    {
        MacPlatform platform;
        MacCaptureBackend backend(&platform);

        QSignalSpy stateSpy(&backend, &CaptureBackend::stateChanged);
        backend.stopRecording();
        QCOMPARE(backend.state(), RecordingState::Idle);
        QCOMPARE(stateSpy.count(), 0);
    }

    void backendPauseResumeGuardedToRecordingState()
    {
        MacPlatform platform;
        MacCaptureBackend backend(&platform);

        // pause/resume only have effect during Recording/Paused — they
        // should be no-ops while Idle.
        backend.pauseRecording();
        QCOMPARE(backend.state(), RecordingState::Idle);
        backend.resumeRecording();
        QCOMPARE(backend.state(), RecordingState::Idle);
    }

    // ── C bridge wiring ────────────────────────────────────────

    void filterReleaseHandlesNull()
    {
        // sc_filter_release must be safe to call with NULL — used in cleanup
        // paths even when we never received a filter.
        sc_filter_release(nullptr);
    }

    void pickerLifecycle()
    {
        // sc_picker_create + destroy without ever showing — verifies the
        // Swift-side Unmanaged.passRetained / takeRetainedValue pairing.
        struct CallbackBag { int unused = 0; } bag;
        SCPickerHandle h = sc_picker_create(
            &bag,
            [](void *, const char *, const char *, void *) {},
            [](void *) {},
            [](void *, const char *) {});
        QVERIFY(h != nullptr);
        sc_picker_destroy(h);
    }

    void recorderLifecycle()
    {
        struct CallbackBag { int unused = 0; } bag;
        SCRecorderHandle h = sc_recorder_create(
            &bag,
            [](void *, const char *) {},
            [](void *, const char *) {},
            [](void *, const char *) {});
        // sc_recorder_create returns NULL on macOS < 15; otherwise non-NULL.
        if (h) {
            sc_recorder_destroy(h);
        }
    }
};

QTEST_MAIN(TstMacCapture)
#include "tst_maccapture.moc"
