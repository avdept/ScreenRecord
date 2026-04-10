#pragma once

#include "ProjectTypes.h"
#include <QString>

namespace screencopy {

struct ExportConfig {
    ExportFormat format = ExportFormat::MP4;
    ExportQuality quality = ExportQuality::Good;
    GifFrameRate gifFrameRate = GifFrameRate::FPS_20;
    GifSizePreset gifSizePreset = GifSizePreset::Medium;
    bool gifLoop = true;
    QString outputPath;
};

struct ExportProgress {
    int currentFrame = 0;
    int totalFrames = 0;
    double percentage = 0.0;
    QString phase;  // "encoding", "muxing", "rendering"
};

} // namespace screencopy
