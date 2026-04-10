#pragma once

#include <QObject>

namespace screencopy {

class AudioCapture : public QObject
{
    Q_OBJECT

public:
    explicit AudioCapture(QObject *parent = nullptr);

    Q_INVOKABLE QStringList availableInputDevices() const;
    Q_INVOKABLE QStringList availableOutputDevices() const;

signals:
    void audioLevelChanged(double level);
};

} // namespace screencopy
