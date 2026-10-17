#include "NodeProject.h"
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QMap>
#include <cmath>

namespace {
constexpr qint64 MaxBytes=256*1024;
bool integer(const QJsonValue &value, int low, int high) {
    const double n=value.toDouble(-1);
    return value.isDouble() && std::isfinite(n) && n==std::floor(n) && n>=low && n<=high;
}
QString validate(const QJsonObject &graph) {
    if(!graph.value("nodes").isArray() || !graph.value("edges").isArray())
        return QStringLiteral("工程缺少节点或连线数组");
    const auto nodes=graph.value("nodes").toArray(), edges=graph.value("edges").toArray();
    if(nodes.size()>24 || edges.size()>24) return QStringLiteral("工程最多24个节点和24条连线");
    const QMap<QString,QSet<QString>> keys={
        {"sample",{"hasDefect"}}, {"screen",{"mode"}}, {"gray",{}},
        {"blur",{"ksize","sigma"}}, {"canny",{"low","high"}},
        {"threshold",{"threshold","otsu"}}, {"morph",{"ksize","op"}},
        {"contours",{"minArea","maxArea"}}, {"qr",{}}
    };
    QMap<int,QString> types;
    for(const auto &value:nodes) {
        if(!value.isObject()) return QStringLiteral("节点必须是对象");
        const auto node=value.toObject();
        if(!integer(node.value("id"),1,1000000000)) return QStringLiteral("节点编号无效");
        const int id=node.value("id").toInt();
        const auto type=node.value("type").toString();
        if(types.contains(id) || !keys.contains(type)) return QStringLiteral("节点编号重复或类型不支持");
        types[id]=type;
        if(!node.value("label").isString() || node.value("label").toString().size()>100 ||
           !integer(node.value("x"),0,1310) || !integer(node.value("y"),0,820) || !node.value("params").isObject())
            return QStringLiteral("节点名称、位置或参数格式无效");
        const auto params=node.value("params").toObject();
        for(auto it=params.begin();it!=params.end();++it) {
            if(!keys[type].contains(it.key())) return QStringLiteral("节点含不支持的参数：%1").arg(it.key());
            const auto key=it.key(); const auto v=it.value();
            if(key=="hasDefect" || key=="otsu") {
                if(!v.isBool()) return QStringLiteral("开关参数必须为布尔值");
            } else if(key=="mode") {
                if(!QStringList{"center","mouse","full"}.contains(v.toString())) return QStringLiteral("屏幕源模式无效");
            } else if(key=="op") {
                if(!QStringList{"Close","Open","Dilate","Erode"}.contains(v.toString())) return QStringLiteral("形态学类型无效");
            } else if(key=="ksize") {
                if(!integer(v,1,51) || v.toInt()%2==0) return QStringLiteral("核大小必须为1–51的奇数");
            } else {
                const double maximum=key=="sigma"?20:(key=="minArea"||key=="maxArea"?200000:255);
                if(!v.isDouble() || !std::isfinite(v.toDouble()) || v.toDouble()<0 || v.toDouble()>maximum)
                    return QStringLiteral("数值参数越界：%1").arg(key);
            }
        }
    }
    QSet<int> inputs;
    for(const auto &value:edges) {
        if(!value.isObject()) return QStringLiteral("连线必须是对象");
        const auto edge=value.toObject();
        if(!integer(edge.value("from"),1,1000000000) || !integer(edge.value("to"),1,1000000000))
            return QStringLiteral("连线端点格式无效");
        const int from=edge.value("from").toInt(),to=edge.value("to").toInt();
        if(!types.contains(from) || !types.contains(to) || from==to || inputs.contains(to) ||
           types[to]=="sample" || types[to]=="screen") return QStringLiteral("连线端点或输入数量无效");
        inputs.insert(to);
    }
    const auto selected=graph.value("selected");
    if(!integer(selected,-1,1000000000) || (selected.toInt()!=-1 && !types.contains(selected.toInt())))
        return QStringLiteral("选中节点不存在");
    // 未连完或含环的草稿也可保存；执行器仍负责检查图是否可运行。
    return {};
}
}
QVariantMap NodeProject::load(const QUrl &url) {
    auto fail=[this](const QString &message){m_status=message;emit changed();return QVariantMap{};};
    if(!url.isLocalFile()) return fail(tr("请选择本机工程文件"));
    QFile file(url.toLocalFile());
    if(!file.open(QIODevice::ReadOnly)) return fail(tr("无法打开工程：%1").arg(file.errorString()));
    if(file.size()>MaxBytes) return fail(tr("工程超过256 KiB上限"));
    const auto bytes=file.read(MaxBytes+1);
    if(file.error()!=QFileDevice::NoError || bytes.size()>MaxBytes) return fail(tr("工程读取失败或文件过大"));
    QJsonParseError parse;
    const auto document=QJsonDocument::fromJson(bytes,&parse);
    if(parse.error!=QJsonParseError::NoError || !document.isObject()) return fail(tr("工程JSON格式无效：%1").arg(parse.errorString()));
    const auto root=document.object();
    if(root.value("format").toString()!="visioncraft.node-project" || !integer(root.value("version"),1,1))
        return fail(tr("不是受支持的VisionCraft节点工程（版本1）"));
    const auto error=validate(root);
    if(!error.isEmpty()) return fail(error);
    m_file=url;m_status=tr("工程已打开：%1").arg(url.toLocalFile());emit changed();
    return QVariantMap{{"nodes",root.value("nodes").toArray().toVariantList()},
                       {"edges",root.value("edges").toArray().toVariantList()},
                       {"selected",root.value("selected").toInt()}};
}
bool NodeProject::save(const QUrl &url,const QVariantMap &graph) {
    auto fail=[this](const QString &message){m_status=message;emit changed();return false;};
    if(!url.isLocalFile()) return fail(tr("请选择本机保存位置"));
    auto root=QJsonObject::fromVariantMap(graph);
    const auto error=validate(root);
    if(!error.isEmpty()) return fail(error);
    root["format"]="visioncraft.node-project";root["version"]=1;
    const auto bytes=QJsonDocument(root).toJson(QJsonDocument::Indented);
    if(bytes.size()>MaxBytes) return fail(tr("工程超过256 KiB上限"));
    QSaveFile file(url.toLocalFile());
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())
        return fail(tr("工程保存失败：%1").arg(file.errorString()));
    m_file=url;m_status=tr("工程已保存：%1").arg(url.toLocalFile());emit changed();return true;
}
void NodeProject::detach() {m_file=QUrl{};m_status=tr("新工程尚未保存");emit changed();}
