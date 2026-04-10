#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QPointF>
#include <QSizeF>
#include <QRectF>

namespace screencopy {

// Aspect ratio presets
enum class AspectRatio {
    Native,
    Ratio_16_9,
    Ratio_9_16,
    Ratio_1_1,
    Ratio_4_3,
    Ratio_4_5,
    Ratio_16_10,
    Ratio_10_16
};

// Export quality levels
enum class ExportQuality {
    Medium,
    Good,
    Source
};

// Export format
enum class ExportFormat {
    MP4,
    GIF
};

// GIF frame rate presets
enum class GifFrameRate {
    FPS_15 = 15,
    FPS_20 = 20,
    FPS_25 = 25,
    FPS_30 = 30
};

// GIF size presets
enum class GifSizePreset {
    Medium,   // 720p max
    Large,    // 1080p max
    Original  // full resolution
};

// Webcam layout
enum class WebcamLayoutPreset {
    PictureInPicture,
    VerticalStack
};

// Webcam mask shape
enum class WebcamMaskShape {
    Rectangle,
    Circle,
    Square,
    Rounded
};

// Annotation types
enum class AnnotationType {
    Text,
    Image,
    Figure
};

// Arrow directions for figure annotations
enum class ArrowDirection {
    Up, Down, Left, Right,
    UpRight, UpLeft, DownRight, DownLeft
};

// Zoom region
struct ZoomRegion {
    qint64 startMs = 0;
    qint64 endMs = 0;
    int depth = 1;           // 1-6, maps to scale factors
    double focusX = 0.5;     // 0-1 normalized
    double focusY = 0.5;     // 0-1 normalized
    bool autoFollow = true;  // follow cursor telemetry

    static constexpr double scaleForDepth(int depth) {
        constexpr double scales[] = {1.0, 1.25, 1.5, 1.8, 2.2, 3.5, 5.0};
        if (depth < 0 || depth > 6) return 1.0;
        return scales[depth];
    }
};

// Trim region (segments to keep)
struct TrimRegion {
    qint64 startMs = 0;
    qint64 endMs = 0;
};

// Speed region
struct SpeedRegion {
    qint64 startMs = 0;
    qint64 endMs = 0;
    double speed = 1.0;  // 0.1 - 16.0
};

// Crop region (normalized 0-1 coordinates)
struct CropRegion {
    double x = 0.0;
    double y = 0.0;
    double width = 1.0;
    double height = 1.0;
};

// Text annotation style
struct TextStyle {
    QString fontFamily = "system-ui";
    int fontSize = 24;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    QString alignment = "center";  // left, center, right
    QString color = "#ffffff";
    QString backgroundColor = "transparent";
};

// Annotation region
struct AnnotationRegion {
    AnnotationType type = AnnotationType::Text;
    qint64 startMs = 0;
    qint64 endMs = 0;
    double x = 0.0;       // 0-1 normalized position
    double y = 0.0;
    double width = 0.2;   // 0-1 normalized size
    double height = 0.1;
    int zIndex = 0;

    // Text-specific
    QString text;
    TextStyle textStyle;

    // Image-specific
    QString imageData;  // data URL or file path

    // Figure-specific
    ArrowDirection arrowDirection = ArrowDirection::Right;
    double strokeWidth = 2.0;
    QString strokeColor = "#ffffff";
};

// Webcam position
struct WebcamPosition {
    double cx = 0.85;  // 0-1 normalized center
    double cy = 0.85;
};

// Cursor telemetry sample
struct CursorSample {
    qint64 timeMs = 0;
    double cx = 0.5;  // 0-1 normalized
    double cy = 0.5;
};

// Recording session metadata
struct RecordingSession {
    QString screenVideoPath;
    QString webcamVideoPath;  // optional
    qint64 createdAt = 0;     // ms since epoch
};

} // namespace screencopy
