#include "AudioProcessor.h"

namespace screencopy {

AudioProcessor::AudioProcessor(QObject *parent)
    : QObject(parent)
{
}

bool AudioProcessor::extractAudio(const QString &videoPath, const QString &outputPath)
{
    // TODO: extract audio stream using libavformat
    Q_UNUSED(videoPath);
    Q_UNUSED(outputPath);
    return false;
}

bool AudioProcessor::applySpeedChange(const QString &inputPath, const QString &outputPath, double speed)
{
    // TODO: use libavfilter atempo filter for speed change
    Q_UNUSED(inputPath);
    Q_UNUSED(outputPath);
    Q_UNUSED(speed);
    return false;
}

} // namespace screencopy
