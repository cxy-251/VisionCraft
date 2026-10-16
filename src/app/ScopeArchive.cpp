#include "ScopeArchive.h"
#include "../device/ScopeFrame.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {
constexpr qint64 MaxBytes=128*1024;
constexpr auto Format="VisionCraft.ScopeFrame";
bool integer(const QJsonValue &v, double lo, double hi)
{
    const double n=v.toDouble(-1);
    return v.isDouble() && std::isfinite(n) && n>=lo && n<=hi && std::floor(n)==n;
}
bool timestamp(const QJsonValue &v)
{
    const auto s=v.toString();
    return s.size()<=40 && s.endsWith('Z') && QDateTime::fromString(s,Qt::ISODateWithMs).isValid();
}
bool metadata(const QJsonObject &a, const QJsonValue &received, const QJsonValue &saved, const QString &source)
{
    const double signal=a.value("signalHz").toDouble(-1), elapsed=a.value("transferMs").toDouble(-1);
    return integer(a.value("count"),32,2048) && integer(a.value("id"),0,4294967295.0) &&
        integer(a.value("vddaMv"),0,5000) && a.value("rate").isDouble() &&
        std::isfinite(signal) && signal>=0 && signal<=200000 &&
        std::isfinite(elapsed) && elapsed>=0 && elapsed<=86400000 &&
        timestamp(received) && timestamp(saved) && !source.isEmpty() && source.size()<=256;
}
}
bool ScopeArchive::fail(const QString &why)
{
    m_status=why; emit statusChanged(); return false;
}
// [region save]
bool ScopeArchive::save(const QUrl &url, const QVariantMap &frame)
{
    if (!url.isLocalFile()) return fail(tr("请选择电脑上的本地文件"));
    QString error;
    const auto checked=ScopeFrame::normalized(frame,&error);
    if (checked.isEmpty()) return fail(error);
    const auto received=QJsonValue(checked.value("receivedAtUtc").toString());
    const auto source=checked.value("source").toString();
    if (!timestamp(received) || source.isEmpty() || source.size()>256)
        return fail(tr("采样帧缺少有效的接收时间或来源"));
    const QJsonObject acquisition{
        {"id",checked.value("id").toDouble()}, {"rate",checked.value("rate").toDouble()},
        {"count",checked.value("count").toInt()}, {"vddaMv",checked.value("vddaMv").toDouble()},
        {"signalHz",checked.value("signalHz").toDouble()}, {"transferMs",checked.value("transferMs").toDouble()}};
    const QJsonObject root{{"format",Format},{"version",1},
        {"savedAtUtc",QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"receivedAtUtc",received},{"source",source},{"acquisition",acquisition},
        {"samples",QJsonArray::fromVariantList(checked.value("samples").toList())}};
    if (!metadata(acquisition,received,root.value("savedAtUtc"),source))
        return fail(tr("采样帧元数据无效"));
    const auto bytes=QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (bytes.size()>MaxBytes) return fail(tr("采样文件超过128 KiB限制"));
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())
        return fail(tr("保存失败：%1").arg(file.errorString()));
    m_status=tr("已保存到电脑：%1").arg(QFileInfo(url.toLocalFile()).fileName());
    emit statusChanged(); return true;
}
// [endregion]
// [region load]
bool ScopeArchive::load(const QUrl &url)
{
    if (!url.isLocalFile()) return fail(tr("请选择电脑上的本地文件"));
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) return fail(tr("打开失败：%1").arg(file.errorString()));
    if (file.size()>MaxBytes) return fail(tr("采样文件超过128 KiB限制"));
    QJsonParseError parse;
    const auto bytes=file.read(MaxBytes+1);
    if (bytes.size()>MaxBytes) return fail(tr("采样文件超过128 KiB限制"));
    const auto doc=QJsonDocument::fromJson(bytes,&parse);
    if (parse.error!=QJsonParseError::NoError || !doc.isObject()) return fail(tr("采样文件不是有效JSON对象"));
    const auto root=doc.object();
    if (root.value("format")!=Format || root.value("version")!=1)
        return fail(tr("不支持的采样文件格式或版本"));
    const auto a=root.value("acquisition").toObject();
    const auto source=root.value("source").toString();
    if (!root.value("samples").isArray() ||
        !metadata(a,root.value("receivedAtUtc"),root.value("savedAtUtc"),source))
        return fail(tr("采样文件元数据无效"));
    for (const auto &v:root.value("samples").toArray())
        if (!integer(v,0,4095)) return fail(tr("采样帧含无效ADC码"));
    QVariantMap frame=a.toVariantMap();
    frame["samples"]=root.value("samples").toArray().toVariantList();
    frame["source"]=source; frame["receivedAtUtc"]=root.value("receivedAtUtc").toString();
    QString error;
    frame=ScopeFrame::normalized(frame,&error);
    if (frame.isEmpty()) return fail(error);
    // Publish only after all checks; a bad file cannot replace the last valid replay.
    m_frame=frame; m_file=QFileInfo(url.toLocalFile()).fileName();
    m_status=tr("已打开离线采样：%1").arg(m_file);
    emit replayChanged(); emit statusChanged(); return true;
}
// [endregion]
void ScopeArchive::closeReplay()
{
    m_frame.clear(); m_file.clear(); m_status.clear();
    emit replayChanged(); emit statusChanged();
}
