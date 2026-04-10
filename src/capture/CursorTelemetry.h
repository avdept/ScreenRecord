#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include "ProjectTypes.h"

namespace screencopy {

class CursorTelemetry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)

public:
    explicit CursorTelemetry(QObject *parent = nullptr);

    bool isRecording() const { return m_recording; }
    const QList<CursorSample> &samples() const { return m_samples; }

    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE void clear();

    bool saveToFile(const QString &filePath) const;
    bool loadFromFile(const QString &filePath);

signals:
    void recordingChanged();

private:
    void captureSample();

    QTimer m_timer;
    QList<CursorSample> m_samples;
    bool m_recording = false;
    qint64 m_startTime = 0;

    static constexpr int SampleIntervalMs = 100;  // 10 Hz
    static constexpr int MaxSamples = 36000;       // 1 hour
};

} // namespace screencopy
