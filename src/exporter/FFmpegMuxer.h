#pragma once

#include <QObject>
#include <QString>

namespace screencopy {

class FFmpegMuxer : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegMuxer(QObject *parent = nullptr);

    bool mux(const QString &videoPath, const QString &audioPath, const QString &outputPath);
};

} // namespace screencopy
