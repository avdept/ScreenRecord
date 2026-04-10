#include "PipeWireCapture.h"

namespace screencopy {

PipeWireCapture::PipeWireCapture(QObject *parent)
    : CaptureBackend(parent)
{
}

bool PipeWireCapture::isAvailable() const
{
    // TODO: check PipeWire availability
    return true;
}

void PipeWireCapture::startRecording(const QString &outputPath)
{
    // TODO: implement PipeWire portal screen capture
    Q_UNUSED(outputPath);
}

void PipeWireCapture::stopRecording()
{
    // TODO
}

void PipeWireCapture::pauseRecording()
{
    // TODO
}

void PipeWireCapture::resumeRecording()
{
    // TODO
}

void PipeWireCapture::cancelRecording()
{
    // TODO
}

} // namespace screencopy
