#include "FFmpegMuxer.h"

namespace screencopy {

FFmpegMuxer::FFmpegMuxer(QObject *parent)
    : QObject(parent)
{
}

bool FFmpegMuxer::mux(const QString &videoPath, const QString &audioPath, const QString &outputPath)
{
    // TODO: implement audio+video muxing using libavformat
    Q_UNUSED(videoPath);
    Q_UNUSED(audioPath);
    Q_UNUSED(outputPath);
    return false;
}

} // namespace screencopy
