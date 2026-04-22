#pragma once

#include <QObject>
#include <QString>

namespace screencopy {

class MacScreenCapture : public QObject
{
    Q_OBJECT

public:
    explicit MacScreenCapture(QObject *parent = nullptr);
    ~MacScreenCapture();

    void showPicker();
    void showDisplayPicker();
    void showWindowPicker();

    // Store/retrieve the last SCContentFilter from source selection (as retained void*)
    void storeFilter(void *retainedFilter);
    void *takeLastFilter();

signals:
    void sourceSelected(const QString &sourceId, const QString &sourceName);
    void selectionCancelled();
    void selectionError(const QString &message);

private:
    void presentPickerWithModes(unsigned long modes);
    void enumerateAndSelectMainDisplay();

    void *m_helper = nullptr;
    void *m_lastFilter = nullptr;
};

} // namespace screencopy
