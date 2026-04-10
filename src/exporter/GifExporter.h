#pragma once

#include <QObject>
#include "ExportTypes.h"

namespace screencopy {

class GifExporter : public QObject
{
    Q_OBJECT

public:
    explicit GifExporter(QObject *parent = nullptr);

    bool exportGif(const QString &inputPath, const QString &outputPath,
                   GifFrameRate frameRate, GifSizePreset sizePreset, bool loop);

signals:
    void progressChanged(double percentage);
};

} // namespace screencopy
