#pragma once

#include <QObject>
#include <QString>

namespace screencopy {

// C++/Qt façade over the Swift-side ScreenCaptureKit picker (see
// MacScreenCaptureBridge.h and src/platform/mac/). Swift delivers callbacks
// on arbitrary SCK dispatch queues; this class marshals them back to the
// Qt main thread and re-emits as Qt signals.
class MacScreenCapture : public QObject
{
    Q_OBJECT

public:
    explicit MacScreenCapture(QObject *parent = nullptr);
    ~MacScreenCapture() override;

    void showPicker();
    void showDisplayPicker();
    void showWindowPicker();

    // Returns the opaque SCContentFilter handle delivered with the last
    // sourceSelected signal. Ownership transfers to the caller, which must
    // eventually release it via sc_filter_release().
    void *takeLastFilter();

signals:
    void sourceSelected(const QString &sourceId, const QString &sourceName);
    void selectionCancelled();
    void selectionError(const QString &message);

private:
    // Static trampolines matching the C ABI — called from Swift on SCK queues.
    static void onSourceSelected(void *ctx,
                                 const char *sourceId,
                                 const char *sourceName,
                                 void *filterHandle);
    static void onSelectionCancelled(void *ctx);
    static void onSelectionError(void *ctx, const char *message);

    void *m_picker = nullptr;      // SCPickerHandle — Swift-owned
    void *m_lastFilter = nullptr;  // SCContentFilter handle — released via sc_filter_release
};

} // namespace screencopy
