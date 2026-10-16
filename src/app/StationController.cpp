#include "StationController.h"

#include "DeviceLink.h"

#include <QDateTime>
#include <QUuid>
#include <cmath>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/imgproc.hpp>

StationController::StationController(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFutureWatcher<Inspector::Result>::finished, this, &StationController::onInspected);
    m_lineTimer.setSingleShot(true);
    connect(&m_lineTimer, &QTimer::timeout, this, &StationController::inspectNow);
    m_grabTimeout.setSingleShot(true);
    m_grabTimeout.setInterval(5000);
    connect(&m_grabTimeout,&QTimer::timeout,this,[this] { cancelCapture(); });
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
    if (m_busy || m_review.busy()) return;
    m_part = m_generator.next(m_defectRate, m_difficulty);
    m_partDifficulty=m_difficulty;
    m_partImage = toQImage(m_part.image);
    emit partChanged();
}

void StationController::setRunning(bool on)
{
    if (on && m_review.busy()) return;
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
    if (m_busy || !m_review.beginCapture())
        return;
    m_pendingTruth=int(m_part.truth);
    m_pendingInspector=m_inspector;
    m_pendingUtc=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    m_pendingSource=tr("工位QML截图 · 随机模拟件 · difficulty=%1").arg(m_partDifficulty);
    m_captureRequested=true;
    m_grabTimeout.start();
    setBusy(true);
    emit grabRequested(++m_captureId);   // QML 截图后调用 inspectGrab
}

void StationController::inspectGrab(const QImage &screenshot, quint64 request)
{
    if (!m_captureRequested || request!=m_captureId) return; // 重复/迟到的回调不能发起第二次检测
    if (screenshot.isNull() || screenshot.size()!=QSize(480,360)) { cancelCapture(); return; }
    m_captureRequested=false;
    m_grabTimeout.stop();
    m_pendingInput=screenshot.convertToFormat(QImage::Format_RGB888);
    // [region thread]
    const cv::Mat mat = toMat(screenshot);
    const Inspector inspector = m_pendingInspector;   // 按值捕获：线程里用的是一份拷贝，主线程改参数不会影响正在跑的检测
    m_watcher.setFuture(QtConcurrent::run([inspector, mat] {
        try { return inspector.inspect(mat); }
        catch(const std::exception &e) { Inspector::Result result; result.error=QString::fromUtf8(e.what()); return result; }
    }));
    // [endregion]
}

void StationController::onInspected()
{
    const Inspector::Result r = m_watcher.result();
    if (!r.error.isEmpty() || r.annotated.empty()) {
        m_review.cancelCapture(); setBusy(false);
        if(m_running) m_lineTimer.start(m_intervalMs);
        return;
    }
    const vc_defect truth = vc_defect(m_pendingTruth);
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
    InspectionSession::Record record;
    record.id=QUuid::createUuid().toString(QUuid::WithoutBraces);
    record.utc=m_pendingUtc; record.source=m_pendingSource; record.truth=int(truth);
    record.input=m_pendingInput; record.annotated=m_resultImage;
    record.recipe=InspectionSession::paramsMap(m_pendingInspector.params);
    record.result=InspectionSession::resultMap(r);
    m_review.append(std::move(record));
    emit resultChanged();
    emit statsChanged();

    if (m_link && m_link->state() == DeviceLink::Connected) {
        m_link->sendResult(r.ok, int(r.defect), int(r.ms + 0.5), m_total, m_ng);
        // 不合格才发缩略图：一张约 38 KB，经 RTT 大约要 1 秒；上一张还没发完就跳过这张
        if (!r.ok)
            m_link->sendImage(m_resultImage);
    }

    setBusy(false);
    nextPart();   // 传送带送下一件
    if (m_running)
        m_lineTimer.start(m_intervalMs);
}

void StationController::cancelCapture(quint64 request)
{
    if (!m_captureRequested || (request && request!=m_captureId)) return;
    m_captureRequested=false; m_grabTimeout.stop();
    m_review.cancelCapture(); setBusy(false);
    if(m_running) m_lineTimer.start(m_intervalMs);
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
    if(m_busy) return;
    m_total = m_ng = m_correct = 0;
    std::fill(&m_confusion[0][0], &m_confusion[0][0] + VC_DEFECT_COUNT * VC_DEFECT_COUNT, 0);
    m_history.clear();
    m_timeline.clear();
    emit statsChanged();
}

// ------------------------------------------------------------------ 配方

void StationController::setLink(DeviceLink *link)
{
    if (m_link == link)
        return;
    if (m_link)
        m_link->disconnect(this);
    m_link = link;
    if (m_link) {
        // 每次连上板子，先读板子上的配方
        connect(m_link, &DeviceLink::stateChanged, this, [this] {
            if (m_link->state() == DeviceLink::Connected)
                loadRecipeFromBoard();
        });
        connect(m_link, &DeviceLink::recipeReceived, this, [this](const QVariantMap &m) {
            if (m.isEmpty())
                return;   // 板子上没有，继续用当前值
            applyRecipe(m);
            m_recipeSource = tr("板子");
            emit recipeChanged();
        });
        connect(m_link, &DeviceLink::recipeSaved, this, [this](bool ok) {
            if (ok) {
                m_recipeSource = tr("板子");
                emit recipeChanged();
            }
        });
    }
    emit settingsChanged();
}

QVariantMap StationController::recipe() const
{
    const Inspector::Params &p = m_inspector.params;
    return {
        {QStringLiteral("surfaceThreshold"), p.surfaceThreshold},
        {QStringLiteral("minDefectArea"), p.minDefectArea},
        {QStringLiteral("maxCenterOffset"), p.maxCenterOffset},
        {QStringLiteral("minChipDepth"), p.minChipDepth},
        {QStringLiteral("scratchElongation"), p.scratchElongation},
    };
}

void StationController::applyRecipe(const QVariantMap &m)
{
    Inspector::Params checked;
    if(InspectionSession::parseParams(m,&checked)) m_inspector.params=checked;
}

void StationController::setRecipeValue(const QString &key, double value)
{
    QVariantMap m = recipe();
    if(!m.contains(key) || !std::isfinite(value)) return;
    m[key] = value;
    Inspector::Params checked;
    if(!InspectionSession::parseParams(m,&checked)) return;
    m_inspector.params=checked;
    m_recipeSource = tr("已修改，未保存到板子");
    emit recipeChanged();
}

void StationController::resetRecipe()
{
    m_inspector.params=Inspector::Params{};
    m_recipeSource=tr("默认值"); emit recipeChanged();
}

void StationController::saveRecipeToBoard()
{
    if (m_link)
        m_link->setRecipe(recipe());
}

void StationController::loadRecipeFromBoard()
{
    if (m_link)
        m_link->getRecipe();
}
