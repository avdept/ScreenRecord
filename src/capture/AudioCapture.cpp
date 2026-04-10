#include "AudioCapture.h"
#include <QMediaDevices>
#include <QAudioDevice>

namespace screencopy {

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
{
}

QStringList AudioCapture::availableInputDevices() const
{
    QStringList devices;
    for (const auto &dev : QMediaDevices::audioInputs())
        devices << dev.description();
    return devices;
}

QStringList AudioCapture::availableOutputDevices() const
{
    QStringList devices;
    for (const auto &dev : QMediaDevices::audioOutputs())
        devices << dev.description();
    return devices;
}

} // namespace screencopy
