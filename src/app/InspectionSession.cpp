#include "InspectionSession.h"
#include "PartGenerator.h"
#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace {
constexpr int MaxRecords=60;
constexpr qint64 MaxBytes=64*1024*1024;
QString outcome(int truth, int predicted)
{
    if (truth==predicted) return QStringLiteral("判对");
    if (truth!=VC_DEFECT_NONE && predicted==VC_DEFECT_NONE) return QStringLiteral("漏检");
    if (truth==VC_DEFECT_NONE) return QStringLiteral("误报");
    return QStringLiteral("类型误判");
}
bool number(const QVariantMap &m, const QString &key, double lo, double hi, bool whole=false)
{
    const auto v=m.value(key);
    if (v.metaType().id()!=QMetaType::Double && v.metaType().id()!=QMetaType::Int && v.metaType().id()!=QMetaType::UInt && v.metaType().id()!=QMetaType::LongLong && v.metaType().id()!=QMetaType::ULongLong) return false;
    bool ok=false; const double n=v.toDouble(&ok);
    return ok && std::isfinite(n) && n>=lo && n<=hi && (!whole || std::floor(n)==n);
}
QByteArray png(const QImage &image)
{
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly);
    return image.save(&buffer,"PNG") ? bytes.toBase64() : QByteArray{};
}
QImage decode(const QJsonValue &value)
{
    if (!value.isString() || value.toString().size()>2*1024*1024) return {};
    const auto decoded=QByteArray::fromBase64Encoding(value.toString().toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
    if (!decoded) return {};
    QByteArray bytes=decoded.decoded; QBuffer buffer(&bytes); buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer,"PNG");
    if (reader.size()!=QSize(480,360)) return {};
    return reader.read().convertToFormat(QImage::Format_RGB888);
}
}
InspectionSession::InspectionSession(QObject *parent):QObject(parent)
{
    connect(&m_watcher,&QFutureWatcher<Work>::finished,this,[this] {
        const auto work=m_watcher.result();
        m_working=false;
        if (work.error.isEmpty()) {
            m_records=work.records; m_selected=m_records.isEmpty() ? -1 : 0; m_filter=0;
            m_showCandidate=!m_records.isEmpty() && !m_records.first().candidate.isEmpty();
            message(tr("已完成 · %1 张图像，记录仅在电脑上").arg(m_records.size()),true);
            emit changed();
        } else message(work.error);
        emit busyChanged();
    });
}
InspectionSession::~InspectionSession() { m_watcher.waitForFinished(); }
bool InspectionSession::message(const QString &text, bool ok) { m_status=text; emit statusChanged(); return ok; }
QImage InspectionSession::toImage(const cv::Mat &mat)
{
    if (mat.empty()) return {};
    cv::Mat rgb; cv::cvtColor(mat,rgb,cv::COLOR_BGR2RGB);
    return QImage(rgb.data,rgb.cols,rgb.rows,int(rgb.step),QImage::Format_RGB888).copy();
}
cv::Mat InspectionSession::toMat(const QImage &image)
{
    const auto rgb=image.convertToFormat(QImage::Format_RGB888);
    cv::Mat view(rgb.height(),rgb.width(),CV_8UC3,const_cast<uchar *>(rgb.constBits()),size_t(rgb.bytesPerLine())), bgr;
    cv::cvtColor(view,bgr,cv::COLOR_RGB2BGR); return bgr;
}
QVariantMap InspectionSession::paramsMap(const Inspector::Params &p)
{
    return {{"surfaceThreshold",p.surfaceThreshold},{"minDefectArea",p.minDefectArea},
        {"maxCenterOffset",p.maxCenterOffset},{"minChipDepth",p.minChipDepth},{"scratchElongation",p.scratchElongation}};
}
bool InspectionSession::parseParams(const QVariantMap &m, Inspector::Params *p)
{
    if (!number(m,"surfaceThreshold",0,255,true) || !number(m,"minDefectArea",1,10000,true) ||
        !number(m,"maxCenterOffset",0,100) || !number(m,"minChipDepth",0,100) || !number(m,"scratchElongation",1,100)) return false;
    p->surfaceThreshold=m.value("surfaceThreshold").toInt(); p->minDefectArea=m.value("minDefectArea").toInt();
    p->maxCenterOffset=m.value("maxCenterOffset").toDouble(); p->minChipDepth=m.value("minChipDepth").toDouble();
    p->scratchElongation=m.value("scratchElongation").toDouble(); return true;
}
QVariantMap InspectionSession::resultMap(const Inspector::Result &r)
{
    return {{"ok",r.ok},{"defect",int(r.defect)},{"defectName",Inspector::defectName(r.defect)},
        {"ms",r.ms},{"measures",r.measures}};
}
bool InspectionSession::beginCapture()
{
    if (busy()) return false;
    m_capturing=true; emit busyChanged(); return true;
}
void InspectionSession::cancelCapture() { m_capturing=false; message(tr("截图未完成或检测失败，请重新检测")); emit busyChanged(); }
void InspectionSession::append(Record r)
{
    // Live capture and batch work are mutually exclusive; dropping an old record is explicit.
    m_capturing=false;
    for (auto &old:m_records) { old.candidate.clear(); old.candidateImage={}; }
    m_showCandidate=false; m_filter=0;
    m_records.prepend(std::move(r));
    while (m_records.size()>MaxRecords) m_records.removeLast();
    m_selected=0;
    message(tr("已保留最近 %1 条；最多60条，请及时保存到电脑").arg(m_records.size()),true);
    emit changed(); emit busyChanged();
}
void InspectionSession::clear()
{
    if (busy()) return;
    m_records.clear(); m_selected=-1; m_showCandidate=false; emit changed(); message(tr("已清空电脑内存中的记录"),true);
}
void InspectionSession::select(int index) { if (index>=0 && index<m_records.size()) { m_selected=index; emit changed(); } }
void InspectionSession::setFilter(int n)
{
    if(n<0 || n>4) return;
    m_filter=n;
    const auto visible=rows();
    bool selectedVisible=false;
    for(const auto &row:visible) selectedVisible|=row.toMap().value("recordIndex").toInt()==m_selected;
    if(!selectedVisible) m_selected=visible.isEmpty() ? -1 : visible.first().toMap().value("recordIndex").toInt();
    emit changed();
}
void InspectionSession::setShowCandidate(bool value) { m_showCandidate=value; setFilter(m_filter); }
QVariantList InspectionSession::rows() const
{
    QVariantList rows;
    for (int i=0;i<m_records.size();++i) {
        const auto &r=m_records[i];
        const auto result=m_showCandidate && !r.candidate.isEmpty() ? r.candidate : r.result;
        const int predicted=result.value("defect").toInt();
        const auto kind=outcome(r.truth,predicted);
        if ((m_filter==1 && r.truth==predicted) || (m_filter==2 && kind!="漏检") ||
            (m_filter==3 && kind!="误报") || (m_filter==4 && kind!="类型误判")) continue;
        rows.append(QVariantMap{{"recordIndex",i},{"id",r.id},{"truthName",Inspector::defectName(vc_defect(r.truth))},
            {"defectName",result.value("defectName")},{"outcome",kind},{"selected",i==m_selected}});
    }
    return rows;
}
QVariantMap InspectionSession::selected() const
{
    if (m_selected<0 || m_selected>=m_records.size()) return {};
    const auto &r=m_records[m_selected];
    return {{"id",r.id},{"utc",r.utc},{"source",r.source},{"truthName",Inspector::defectName(vc_defect(r.truth))},
        {"recipe",r.recipe},{"original",r.result},{"candidate",r.candidate},
        {"outcome",outcome(r.truth,(m_showCandidate && !r.candidate.isEmpty() ? r.candidate : r.result).value("defect").toInt())}};
}
QImage InspectionSession::inputImage() const { return m_selected>=0 ? m_records[m_selected].input : QImage{}; }
QImage InspectionSession::resultImage() const
{
    if (m_selected<0) return {};
    const auto &r=m_records[m_selected];
    return m_showCandidate && !r.candidate.isEmpty() ? r.candidateImage : r.annotated;
}
// [region metrics]
QVariantMap InspectionSession::metrics(bool candidate) const
{
    int correct=0, good=0, bad=0, escape=0, reject=0, misclass=0, count=0;
    double ms=0;
    int confusion[6][6]={};
    for (const auto &r:m_records) {
        const auto result=candidate ? r.candidate : r.result;
        if (result.isEmpty()) continue;
        const int p=result.value("defect").toInt();
        ++count; ++confusion[r.truth][p]; ms+=result.value("ms").toDouble(); correct+=r.truth==p;
        if (r.truth==0) { ++good; reject+=p!=0; }
        else { ++bad; escape+=p==0; misclass+=p!=0 && p!=r.truth; }
    }
    QVariantList matrix;
    for(const auto &row:confusion) { QVariantList cells; for(int n:row) cells.append(n); matrix.append(QVariant(cells)); }
    return {{"total",count},{"correct",correct},{"good",good},{"bad",bad},{"escape",escape},{"falseReject",reject},{"misclass",misclass},
        {"accuracy",count ? QVariant(double(correct)/count) : QVariant{}},
        {"escapeRate",bad ? QVariant(double(escape)/bad) : QVariant{}},
        {"falseRejectRate",good ? QVariant(double(reject)/good) : QVariant{}},
        {"meanMs",count ? ms/count : 0},{"confusion",matrix}};
}
// [endregion]
QVariantMap InspectionSession::summary() const { return metrics(false); }
QVariantMap InspectionSession::comparison() const { return metrics(true); }
// [region corpus]
bool InspectionSession::createCorpus(int seed, double difficulty, int perClass)
{
    if (busy()) return message(tr("请先停止产线并等待当前检测完成"));
    if (seed<0 || !std::isfinite(difficulty) || difficulty<0 || difficulty>1 || perClass<1 || perClass>12)
        return message(tr("样本参数无效：种子非负、难度0–1、每类1–12张"));
    m_working=true; emit busyChanged(); message(tr("正在生成固定样本并检测…"));
    m_watcher.setFuture(QtConcurrent::run([seed,difficulty,perClass] {
        Work work;
        try {
            PartGenerator generator{unsigned(seed)}; Inspector inspector;
            for(int t=0;t<=VC_DEFECT_OFFSET;++t) for(int i=0;i<perClass;++i) {
                const auto part=generator.make(vc_defect(t),difficulty);
                const auto result=inspector.inspect(part.image);
                Record r; r.id=QStringLiteral("s%1-c%2-%3").arg(seed).arg(t).arg(i);
                r.utc=QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
                r.source=QStringLiteral("固定模拟样本 · seed=%1 · difficulty=%2 · generator-v2").arg(seed).arg(difficulty);
                r.truth=t; r.input=toImage(part.image); r.annotated=toImage(result.annotated);
                r.recipe=paramsMap(inspector.params); r.result=resultMap(result); work.records.append(std::move(r));
            }
        } catch(const std::exception &e) { work.error=QString::fromUtf8(e.what()); }
        return work;
    }));
    return true;
}
// [endregion]
// [region compare]
bool InspectionSession::compare(const QVariantMap &recipe)
{
    if (busy() || m_records.isEmpty()) return message(tr("请先停止产线并准备检测记录"));
    Inspector inspector;
    if (!parseParams(recipe,&inspector.params)) return message(tr("检测配方参数无效"));
    m_working=true; emit busyChanged(); message(tr("正在用当前配方重新检测同一批图像…"));
    m_watcher.setFuture(QtConcurrent::run([records=m_records,inspector] () mutable {
        Work work;
        try {
            for(auto &r:records) {
                const auto result=inspector.inspect(toMat(r.input));
                r.candidate=resultMap(result); r.candidate["recipe"]=paramsMap(inspector.params);
                r.candidateImage=toImage(result.annotated);
            }
            work.records=std::move(records);
        } catch(const std::exception &e) { work.error=QString::fromUtf8(e.what()); }
        return work;
    }));
    return true;
}
// [endregion]
// [region archive]
bool InspectionSession::save(const QUrl &url)
{
    if (busy() || m_records.isEmpty()) return message(tr("请先停止产线并准备完整记录"));
    if (!url.isLocalFile()) return message(tr("请选择电脑上的本地文件"));
    QJsonArray records;
    for(const auto &r:m_records) {
        const auto input=png(r.input), annotated=png(r.annotated);
        if(input.isEmpty() || annotated.isEmpty()) return message(tr("图像编码失败"));
        QJsonObject object{{"id",r.id},{"utc",r.utc},{"source",r.source},{"truth",r.truth},
            {"recipe",QJsonObject::fromVariantMap(r.recipe)},{"result",QJsonObject::fromVariantMap(r.result)},
            {"inputPng",QString::fromLatin1(input)},{"annotatedPng",QString::fromLatin1(annotated)}};
        if(!r.candidate.isEmpty()) {
            const auto candidateImage=png(r.candidateImage);
            if(candidateImage.isEmpty()) return message(tr("对比图像编码失败"));
            object["candidate"]=QJsonObject::fromVariantMap(r.candidate);
            object["candidatePng"]=QString::fromLatin1(candidateImage);
        }
        records.append(object);
    }
    const auto bytes=QJsonDocument(QJsonObject{{"format","VisionCraft.InspectionSession"},{"version",1},{"records",records}}).toJson(QJsonDocument::Compact);
    if(bytes.size()>MaxBytes) return message(tr("记录文件超过64 MiB限制"));
    QSaveFile file(url.toLocalFile());
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) return message(tr("保存失败：%1").arg(file.errorString()));
    return message(tr("已保存 %1 条记录及已有对比结果到电脑").arg(m_records.size()),true);
}
bool InspectionSession::load(const QUrl &url)
{
    if(busy()) return message(tr("请先停止产线并等待当前检测完成"));
    if(!url.isLocalFile()) return message(tr("请选择电脑上的本地文件"));
    QFile file(url.toLocalFile());
    if(!file.open(QIODevice::ReadOnly)) return message(tr("打开失败：%1").arg(file.errorString()));
    if(file.size()>MaxBytes) return message(tr("记录文件超过64 MiB限制"));
    const auto bytes=file.read(MaxBytes+1); QJsonParseError error;
    const auto doc=QJsonDocument::fromJson(bytes,&error); const auto root=doc.object();
    if(bytes.size()>MaxBytes || error.error!=QJsonParseError::NoError || root.value("format")!="VisionCraft.InspectionSession" ||
        root.value("version")!=1 || !root.value("records").isArray()) return message(tr("不支持或损坏的检测记录文件"));
    const auto array=root.value("records").toArray();
    if(array.isEmpty() || array.size()>MaxRecords) return message(tr("记录数必须为1–60"));
    QList<Record> records; QSet<QString> ids;
    for(const auto &value:array) {
        const auto o=value.toObject(); Record r; Inspector::Params params;
        r.id=o.value("id").toString(); r.utc=o.value("utc").toString(); r.source=o.value("source").toString();
        r.recipe=o.value("recipe").toObject().toVariantMap(); r.result=o.value("result").toObject().toVariantMap();
        const auto truth=o.value("truth");
        if(r.id.isEmpty() || r.id.size()>128 || ids.contains(r.id) || r.source.isEmpty() || r.source.size()>512 ||
            r.utc.size()>40 || !r.utc.endsWith('Z') || !QDateTime::fromString(r.utc,Qt::ISODateWithMs).isValid() ||
            !truth.isDouble() || truth.toDouble()<0 || truth.toDouble()>4 || std::floor(truth.toDouble())!=truth.toDouble() ||
            !parseParams(r.recipe,&params) || !number(r.result,"defect",0,5,true) || !number(r.result,"ms",0,86400000) ||
            r.result.value("ok").metaType().id()!=QMetaType::Bool ||
            r.result.value("ok").toBool()!=(r.result.value("defect").toInt()==0) || !o.value("result").toObject().value("measures").isObject())
            return message(tr("记录元数据或配方无效；保留原有记录"));
        for(const auto &v:r.result.value("measures").toMap()) {
            if(!number({{"v",v}},"v",-1e12,1e12)) return message(tr("测量值无效"));
        }
        r.truth=truth.toInt(); ids.insert(r.id);
        r.result["defectName"]=Inspector::defectName(vc_defect(r.result.value("defect").toInt()));
        r.input=decode(o.value("inputPng")); r.annotated=decode(o.value("annotatedPng"));
        if(r.input.isNull() || r.annotated.isNull()) return message(tr("图像损坏或尺寸不是480×360；保留原有记录"));
        if(o.contains("candidate")) {
            r.candidate=o.value("candidate").toObject().toVariantMap();
            if(!number(r.candidate,"defect",0,5,true) || !number(r.candidate,"ms",0,86400000) ||
                r.candidate.value("ok").metaType().id()!=QMetaType::Bool ||
                r.candidate.value("ok").toBool()!=(r.candidate.value("defect").toInt()==0) ||
                !parseParams(r.candidate.value("recipe").toMap(),&params) ||
                !o.value("candidate").toObject().value("measures").isObject()) return message(tr("对比结果无效"));
            for(const auto &v:r.candidate.value("measures").toMap())
                if(!number({{"v",v}},"v",-1e12,1e12)) return message(tr("对比测量值无效"));
            r.candidate["defectName"]=Inspector::defectName(vc_defect(r.candidate.value("defect").toInt()));
            r.candidateImage=decode(o.value("candidatePng"));
            if(r.candidateImage.isNull()) return message(tr("对比图像损坏"));
        }
        records.append(std::move(r));
    }
    // A comparison always covers the entire cohort; partial candidate data is invalid.
    const bool hasCandidate=!records.first().candidate.isEmpty();
    for(const auto &r:records)
        if(r.candidate.isEmpty()==hasCandidate) return message(tr("对比结果不完整"));
    m_records=std::move(records); m_selected=0; m_filter=0; m_showCandidate=false;
    emit changed(); return message(tr("已打开 %1 条离线记录；可按当前配方重新检测").arg(m_records.size()),true);
}
// [endregion]
