#include "ProjectData.h"
#include <QJsonArray>

namespace screencopy {

QJsonObject ProjectData::toJson() const
{
    QJsonObject obj;
    obj["version"] = version;

    QJsonObject media;
    media["screenVideoPath"] = screenVideoPath;
    if (!webcamVideoPath.isEmpty())
        media["webcamVideoPath"] = webcamVideoPath;
    obj["media"] = media;

    // TODO: serialize editor state
    QJsonObject editorObj;
    editorObj["wallpaper"] = editor.wallpaper;
    editorObj["shadowIntensity"] = editor.shadowIntensity;
    editorObj["showBlur"] = editor.showBlur;
    editorObj["motionBlurAmount"] = editor.motionBlurAmount;
    editorObj["borderRadius"] = editor.borderRadius;
    editorObj["padding"] = editor.padding;
    obj["editor"] = editorObj;

    return obj;
}

ProjectData ProjectData::fromJson(const QJsonObject &obj)
{
    ProjectData data;
    data.version = obj["version"].toInt(CurrentVersion);

    auto media = obj["media"].toObject();
    data.screenVideoPath = media["screenVideoPath"].toString();
    data.webcamVideoPath = media["webcamVideoPath"].toString();

    if (obj.contains("editor"))
        data.editor = normalizeEditorState(obj["editor"].toObject());

    return data;
}

EditorState ProjectData::normalizeEditorState(const QJsonObject &obj)
{
    EditorState state;
    state.wallpaper = obj["wallpaper"].toString("wallpaper1");
    state.shadowIntensity = obj["shadowIntensity"].toDouble(0.5);
    state.showBlur = obj["showBlur"].toBool(false);
    state.motionBlurAmount = obj["motionBlurAmount"].toDouble(0.0);
    state.borderRadius = obj["borderRadius"].toDouble(12.0);
    state.padding = obj["padding"].toDouble(50.0);

    // TODO: deserialize regions, crop, webcam settings
    return state;
}

} // namespace screencopy
