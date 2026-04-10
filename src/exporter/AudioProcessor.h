#pragma once

#include <QObject>
#include <QString>

namespace screencopy {

class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    explicit AudioProcessor(QObject *parent = nullptr);

    bool extractAudio(const QString &videoPath, const QString &outputPath);
    bool applySpeedChange(const QString &inputPath, const QString &outputPath, double speed);
};

} // namespace screencopy
