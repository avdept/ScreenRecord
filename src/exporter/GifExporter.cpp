#include "GifExporter.h"

namespace screencopy {

GifExporter::GifExporter(QObject *parent)
    : QObject(parent)
{
}

bool GifExporter::exportGif(const QString &inputPath, const QString &outputPath,
                            GifFrameRate frameRate, GifSizePreset sizePreset, bool loop)
{
    // TODO: implement using libavfilter palettegen + paletteuse filter graph
    Q_UNUSED(inputPath);
    Q_UNUSED(outputPath);
    Q_UNUSED(frameRate);
    Q_UNUSED(sizePreset);
    Q_UNUSED(loop);
    return false;
}

} // namespace screencopy
