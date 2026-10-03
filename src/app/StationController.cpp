#include "StationController.h"

#include "DeviceLink.h"

#include <QDateTime>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/imgproc.hpp>

StationController::StationController(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<Inspector::Result>::finished, this, &StationController::onInspected);
    m_lineTimer.setSingleShot(true);
    connect(&m_lineTimer, &QTimer::timeout, this, &StationController::inspectNow);
    nextPart();
}

// [region toqimage]
QImage StationController::toQImage(const cv::Mat &bgr)
{
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    // QImage 不拥有 rgb 的内存，copy() 一份，rgb 出了作用域也没关系
    return QImage(rgb.data, rgb.cols, rgb.rows, int(rgb.step), QImage::Format_RGB888).copy();
}

cv::Mat StationController::toMat(const QImage &image)
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    const cv::Mat view(rgb.height(), rgb.width(), CV_8UC3, const_cast<uchar *>(rgb.constBits()), size_t(rgb.bytesPerLine()));
    cv::Mat bgr;
    cv::cvtColor(view, bgr, cv::COLOR_RGB2BGR);   // 转换时会复制一份，不再引用 rgb 的内存
    return bgr;
}
// [endregion]

void StationController::nextPart()
{
    m_part = m_generator.next(m_defectRate, m_difficulty);
    m_partImage = toQImage(m_part.image);
    emit partChanged();
}

void StationController::setRunning(bool on)
{
    if (m_running == on)
        return;
    m_running = on;
    if (on && !m_busy)
        m_lineTimer.start(m_intervalMs);
    else if (!on)
        m_lineTimer.stop();
    emit runningChanged();
}

void StationController::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    emit busyChanged();
}

void StationController::inspectNow()
{
    if (m_busy)
        return;
    setBusy(true);
    emit grabRequested();   // QML 截图后调用 inspectGrab
}

void StationController::inspectGrab(const QImage &screenshot)
{
    if (screenshot.isNull()) {
        setBusy(false);
        return;
    }
    // [region thread]
    const cv::Mat mat = toMat(screenshot);
    const Inspector inspector = m_inspector;   // 按值捕获：线程里用的是一份拷贝，主线程改参数不会影响正在跑的检测
    m_watcher.setFuture(QtConcurrent::run([inspector, mat] { return inspector.inspect(mat); }));
    // [endregion]
}

void StationController::onInspected()
{
    const Inspector::Result r = m_watcher.result();
    const vc_defect truth = m_part.truth;
    const bool correct = r.defect == truth;

    m_total++;
    if (!r.ok)
        m_ng++;
    if (correct)
        m_correct++;
    m_confusion[truth][r.defect]++;

    m_resultImage = toQImage(r.annotated);
    m_lastResult = {
        {QStringLiteral("ok"), r.ok},
        {QStringLiteral("defect"), int(r.defect)},
        {QStringLiteral("defectName"), Inspector::defectName(r.defect)},
        {QStringLiteral("truth"), int(truth)},
        {QStringLiteral("truthName"), Inspector::defectName(truth)},
        {QStringLiteral("correct"), correct},
        {QStringLiteral("ms"), r.ms},
        {QStringLiteral("measures"), r.measures},
    };
    m_timeline.append(QVariantMap{{QStringLiteral("t"), QDateTime::currentMSecsSinceEpoch()},
                                  {QStringLiteral("ok"), r.ok},
                                  {QStringLiteral("correct"), correct}});
    if (m_timeline.size() > 2000)
        m_timeline.removeFirst();
    m_history.prepend(m_lastResult);
    while (m_history.size() > 14)
        m_history.removeLast();
    emit resultChanged();
    emit statsChanged();

    if (m_link && m_link->state() == DeviceLink::Connected) {
        m_link->sendResult(r.ok, int(r.defect), int(r.ms + 0.5), m_total, m_ng);
        // 不合格才发缩略图：一张约 38 KB，经 RTT 大约要 1 秒；上一张还没发完就跳过这张
        if (!r.ok)
            m_link->sendImage(m_resultImage);
    }

    nextPart();   // 传送带送下一件
    setBusy(false);
    if (m_running)
        m_lineTimer.start(m_intervalMs);
}

QVariantMap StationController::stats() const
{
    QVariantList confusion;
    for (int t = 0; t <= VC_DEFECT_OFFSET; ++t) {
        QVariantList row;
        for (int p = 0; p < VC_DEFECT_COUNT; ++p)
            row << m_confusion[t][p];
        confusion << QVariant(row);
    }
    return {
        {QStringLiteral("total"), m_total},
        {QStringLiteral("ng"), m_ng},
        {QStringLiteral("ok"), m_total - m_ng},
        {QStringLiteral("correct"), m_correct},
        {QStringLiteral("accuracy"), m_total ? double(m_correct) / m_total : 0.0},
        {QStringLiteral("confusion"), confusion},
    };
}

void StationController::resetStats()
{
    m_total = m_ng = m_correct = 0;
    std::fill(&m_confusion[0][0], &m_confusion[0][0] + VC_DEFECT_COUNT * VC_DEFECT_COUNT, 0);
    m_history.clear();
    m_timeline.clear();
    emit statsChanged();
}
