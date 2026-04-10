#pragma once

#include "ProjectTypes.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace screencopy {

struct EditorState {
    QString wallpaper = "wallpaper1";
    double shadowIntensity = 0.0;
    bool showBlur = false;
    double motionBlurAmount = 0.0;
    double borderRadius = 0.0;
    double padding = 50.0;
    CropRegion cropRegion;
    QList<ZoomRegion> zoomRegions;
    QList<TrimRegion> trimRegions;
    QList<SpeedRegion> speedRegions;
    QList<AnnotationRegion> annotationRegions;
    AspectRatio aspectRatio = AspectRatio::Ratio_16_9;
    WebcamLayoutPreset webcamLayout = WebcamLayoutPreset::PictureInPicture;
    WebcamMaskShape webcamMask = WebcamMaskShape::Rounded;
    std::optional<WebcamPosition> webcamPosition;
    ExportQuality exportQuality = ExportQuality::Good;
    ExportFormat exportFormat = ExportFormat::MP4;
    GifFrameRate gifFrameRate = GifFrameRate::FPS_20;
    bool gifLoop = true;
    GifSizePreset gifSizePreset = GifSizePreset::Medium;
};

struct ProjectData {
    static constexpr int CurrentVersion = 2;

    int version = CurrentVersion;
    QString screenVideoPath;
    QString webcamVideoPath;
    EditorState editor;

    QJsonObject toJson() const;
    static ProjectData fromJson(const QJsonObject &obj);
    static EditorState normalizeEditorState(const QJsonObject &obj);
};

} // namespace screencopy
