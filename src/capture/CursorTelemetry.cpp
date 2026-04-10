#include "CursorTelemetry.h"
#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace screencopy {

CursorTelemetry::CursorTelemetry(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(SampleIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &CursorTelemetry::captureSample);
}

void CursorTelemetry::startRecording()
{
    m_samples.clear();
    m_recording = true;
    m_startTime = 0;
    m_timer.start();
    emit recordingChanged();
}

void CursorTelemetry::stopRecording()
{
    m_timer.stop();
    m_recording = false;
    emit recordingChanged();
}

void CursorTelemetry::clear()
{
    m_samples.clear();
}

void CursorTelemetry::captureSample()
{
    if (m_samples.size() >= MaxSamples)
        return;

    auto *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    auto pos = QCursor::pos();
    auto geom = screen->geometry();

    CursorSample sample;
    if (m_samples.isEmpty()) {
        m_startTime = 0;
        sample.timeMs = 0;
    } else {
        sample.timeMs = m_samples.last().timeMs + SampleIntervalMs;
    }
    sample.cx = qBound(0.0, static_cast<double>(pos.x() - geom.x()) / geom.width(), 1.0);
    sample.cy = qBound(0.0, static_cast<double>(pos.y() - geom.y()) / geom.height(), 1.0);

    m_samples.append(sample);
}

bool CursorTelemetry::saveToFile(const QString &filePath) const
{
    QJsonArray arr;
    for (const auto &s : m_samples) {
        QJsonObject obj;
        obj["timeMs"] = s.timeMs;
        obj["cx"] = s.cx;
        obj["cy"] = s.cy;
        arr.append(obj);
    }

    QJsonObject root;
    root["version"] = 1;
    root["samples"] = arr;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    return true;
}

bool CursorTelemetry::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    auto doc = QJsonDocument::fromJson(file.readAll());
    auto arr = doc.object()["samples"].toArray();

    m_samples.clear();
    m_samples.reserve(arr.size());
    for (const auto &val : arr) {
        auto obj = val.toObject();
        CursorSample s;
        s.timeMs = obj["timeMs"].toInteger();
        s.cx = obj["cx"].toDouble();
        s.cy = obj["cy"].toDouble();
        m_samples.append(s);
    }
    return true;
}

} // namespace screencopy
