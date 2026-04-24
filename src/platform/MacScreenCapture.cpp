#include "MacScreenCapture.h"
#include "MacScreenCaptureBridge.h"

#include <QMetaObject>
#include <QString>

namespace screencopy {

MacScreenCapture::MacScreenCapture(QObject *parent)
    : QObject(parent)
{
    m_picker = sc_picker_create(this,
                                &MacScreenCapture::onSourceSelected,
                                &MacScreenCapture::onSelectionCancelled,
                                &MacScreenCapture::onSelectionError);
}

MacScreenCapture::~MacScreenCapture()
{
    if (m_lastFilter) {
        sc_filter_release(m_lastFilter);
        m_lastFilter = nullptr;
    }
    if (m_picker) {
        sc_picker_destroy(static_cast<SCPickerHandle>(m_picker));
        m_picker = nullptr;
    }
}

void MacScreenCapture::showPicker()
{
    if (m_picker)
        sc_picker_show_all(static_cast<SCPickerHandle>(m_picker));
}

void MacScreenCapture::showDisplayPicker()
{
    if (m_picker)
        sc_picker_show_display(static_cast<SCPickerHandle>(m_picker));
}

void MacScreenCapture::showWindowPicker()
{
    if (m_picker)
        sc_picker_show_window(static_cast<SCPickerHandle>(m_picker));
}

void *MacScreenCapture::takeLastFilter()
{
    void *f = m_lastFilter;
    m_lastFilter = nullptr;
    return f;
}

// ---------- Static C trampolines (called from Swift on SCK queue) ----------

void MacScreenCapture::onSourceSelected(void *ctx,
                                        const char *sourceId,
                                        const char *sourceName,
                                        void *filterHandle)
{
    auto *self = static_cast<MacScreenCapture *>(ctx);
    if (!self)
        return;

    QString id = QString::fromUtf8(sourceId);
    QString name = QString::fromUtf8(sourceName);

    QMetaObject::invokeMethod(self, [self, id, name, filterHandle]() {
        // Release any previously stored filter and take ownership of the new one.
        if (self->m_lastFilter)
            sc_filter_release(self->m_lastFilter);
        self->m_lastFilter = filterHandle;

        emit self->sourceSelected(id, name);
    }, Qt::QueuedConnection);
}

void MacScreenCapture::onSelectionCancelled(void *ctx)
{
    auto *self = static_cast<MacScreenCapture *>(ctx);
    if (!self)
        return;

    QMetaObject::invokeMethod(self, [self]() {
        emit self->selectionCancelled();
    }, Qt::QueuedConnection);
}

void MacScreenCapture::onSelectionError(void *ctx, const char *message)
{
    auto *self = static_cast<MacScreenCapture *>(ctx);
    if (!self)
        return;

    QString msg = QString::fromUtf8(message);
    QMetaObject::invokeMethod(self, [self, msg]() {
        emit self->selectionError(msg);
    }, Qt::QueuedConnection);
}

} // namespace screencopy
