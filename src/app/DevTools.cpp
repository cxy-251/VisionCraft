#include "DevTools.h"
#include <nlohmann/json.hpp>
#include <QSysInfo>
#include <QThread>
#include <QGuiApplication>
#include <QScreen>
#include <opencv2/core/version.hpp>
void DevTools::formatJson(const QString &text,bool compact){
    if(text.size()>1024*1024){m_status="输入超过1 Mi字符上限";emit changed();return;}
    try{const auto object=nlohmann::json::parse(text.toUtf8().toStdString());m_output=QString::fromStdString(object.dump(compact?-1:4));m_status="JSON有效";}
    catch(const std::exception&e){m_status=QString::fromUtf8(e.what());}emit changed();
}
QString DevTools::systemInfo()const{
    QString compiler;
#if defined(__clang__)
    compiler="Clang " __clang_version__;
#elif defined(__GNUC__)
    compiler="GCC " __VERSION__;
#elif defined(_MSC_VER)
    compiler=QString("MSVC %1").arg(_MSC_VER);
#endif
    auto *screen=QGuiApplication::primaryScreen();
    return QString("系统：%1\n架构：%2\nQt：%3\nOpenCV：%4\n编译器：%5\n逻辑线程：%6\n主屏：%7\n主机：%8")
        .arg(QSysInfo::prettyProductName(),QSysInfo::currentCpuArchitecture(),qVersion(),CV_VERSION,compiler)
        .arg(QThread::idealThreadCount()).arg(screen?QString("%1 × %2 · DPR %3").arg(screen->size().width()).arg(screen->size().height()).arg(screen->devicePixelRatio()):"无").arg(QSysInfo::machineHostName());
}
