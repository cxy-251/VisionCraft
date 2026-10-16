#pragma once
#include <QVariantMap>
#include <QString>
#include <algorithm>
#include <cmath>

// Pure Qt Core: live acquisition and file replay share validation/statistics.
namespace ScopeFrame {
inline QVariantMap normalized(QVariantMap frame, QString *error = nullptr)
{
    auto bad = [&](const QString &why) { if (error) *error=why; return QVariantMap{}; };
    const auto samples=frame.value("samples").toList();
    bool rateOk=false, countOk=false;
    const double rate=frame.value("rate").toDouble(&rateOk);
    const double count=frame.value("count").toDouble(&countOk);
    if (!rateOk || !countOk || !std::isfinite(rate) || rate<10000 || rate>200000 ||
        samples.size()<32 || samples.size()>2048 || count!=samples.size())
        return bad(QStringLiteral("采样率、点数或完整帧长度无效"));
    double sum=0, lo=4095, hi=0;
    for (const auto &sample:samples) {
        const int type=sample.metaType().id();
        if (type==QMetaType::Bool || type==QMetaType::QString || type==QMetaType::QByteArray)
            return bad(QStringLiteral("采样帧含非数值ADC码"));
        bool ok=false; const double v=sample.toDouble(&ok);
        if (!ok || !std::isfinite(v) || v<0 || v>4095 || std::floor(v)!=v)
            return bad(QStringLiteral("采样帧含无效ADC码"));
        sum+=v; lo=std::min(lo,v); hi=std::max(hi,v);
    }
    const double mean=sum/samples.size();
    const double hyst=std::max(8.0,(hi-lo)*.05), threshold=mean+hyst;
    QList<double> crossings;
    bool armed=samples.first().toDouble()<mean-hyst;
    for (int i=1;i<samples.size();++i) {
        const double a=samples[i-1].toDouble(), b=samples[i].toDouble();
        if (b<mean-hyst) armed=true;
        if (armed && a<threshold && b>=threshold) {
            crossings.append(i-1+(threshold-a)/(b-a)); armed=false;
        }
    }
    frame["min"]=lo; frame["max"]=hi; frame["mean"]=mean;
    frame["measuredHz"]=crossings.size()>=2 ? rate*(crossings.size()-1)/(crossings.last()-crossings.first()) : 0;
    if (error) error->clear();
    return frame;
}
}
