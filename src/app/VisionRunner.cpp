#include "VisionRunner.h"
#include "LabAlgorithms.h"
#include "core/ScreenCapture.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QCursor>
#include <QElapsedTimer>
#include <QSaveFile>
#include <QDateTime>
#include <QSet>
#include <stdexcept>

namespace {
QImage capture(const QString &kind) {
    QScreen *screen=QGuiApplication::primaryScreen();
    if(kind=="mouse") screen=QGuiApplication::screenAt(QCursor::pos());
    if(!screen) return {};
    const auto size=screen->size();
    if(kind=="full") return screen->grabWindow(0).toImage().scaled(1280,720,Qt::KeepAspectRatio,Qt::FastTransformation);
    const QSize region=QSize(kind=="mouse"?640:800,kind=="mouse"?400:500).boundedTo(size);
    QPoint center=kind=="mouse"?QCursor::pos()-screen->geometry().topLeft():QPoint(size.width()/2,size.height()/2);
    const int x=qBound(0,center.x()-region.width()/2,size.width()-region.width());
    const int y=qBound(0,center.y()-region.height()/2,size.height()-region.height());
    return screen->grabWindow(0,x,y,region.width(),region.height()).toImage();
}
}
VisionRunner::VisionRunner(QObject *parent):QObject(parent) {
    connect(this,&VisionRunner::inputsChanged,this,[this]{++m_revision; emit changed();});
    connect(&m_timer,&QTimer::timeout,this,&VisionRunner::run);
    connect(&m_watcher,&QFutureWatcher<Job>::finished,this,[this]{
        const auto job=m_watcher.result(); m_busy=false;
        if(job.saved) {
            m_status=job.error.isEmpty()?"PNG已保存":job.error; emit changed(); return;
        }
        if(job.revision==m_revision) {
            if(job.error.isEmpty()) {
                m_result=job.image; m_captured=job.source; m_nodes=job.nodes; m_summary=job.summary;
                if(running() && m_streamTime.isValid())m_summary["fps"]=++m_frameCount*1000.0/qMax(qint64(1),m_streamTime.elapsed());
                m_summary["ms"]=job.ms; m_resultRevision=m_revision; ++m_generation;
                m_status=QString("完成 · %1 ms").arg(job.ms,0,'f',1);
                QVariantMap record=m_summary; record["time"]=QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
                m_history.prepend(record); while(m_history.size()>40) m_history.removeLast();
            } else { stop(); m_status=job.error; }
        } else m_status="输入或参数已变更，请重新运行";
        emit changed();
    });
}
VisionRunner::~VisionRunner(){m_timer.stop(); m_watcher.waitForFinished();}
void VisionRunner::run() {
    if(m_busy) return; // 按完成速度处理，丢弃定时器的过期采样，不堆积帧。
    QImage input=m_input;
    auto options=m_options;
    const auto source=m_options.value("capture","input").toString();
    if(source!="input") {
        input=capture(source);
        if(input.isNull()) {stop();m_status="屏幕捕获失败；请检查桌面会话的捕获权限";emit changed();return;}
    }
    if(m_mode=="graph") {
        QVariantMap screens; QMap<QString,QImage> snapshots;
        const auto nodes=options.value("nodes").toList();
        if(nodes.size()<=24)for(const auto &value:nodes){const auto node=value.toMap();if(node.value("type").toString()!="screen")continue;
            const auto kind=node.value("params").toMap().value("mode","center").toString();
            if(!snapshots.contains(kind)) snapshots[kind]=capture(kind);
            screens[QString::number(node.value("id").toInt())]=snapshots[kind];
        }
        options["screens"]=screens;
    }
    m_busy=true; m_status="处理中…"; emit changed();
    m_watcher.setFuture(QtConcurrent::run(&VisionRunner::process,input,m_pattern,m_mode,options,m_revision));
}
void VisionRunner::start(int fps){ if(m_timer.isActive()) return; m_streamTime.start(); m_frameCount=0; m_timer.start(1000/qBound(1,fps,60)); emit changed(); run(); }
void VisionRunner::stop(){
    if(!m_timer.isActive())return;
    const bool wasCurrent=resultCurrent();m_timer.stop();
    if(m_busy){++m_revision;if(wasCurrent)m_resultRevision=m_revision;}
    m_status="连续处理已停止";emit changed();
}
void VisionRunner::centerTemplate(){
    if(m_input.isNull())return;
    const int w=qMax(1,qMin(160,m_input.width()/4)),h=qMax(1,qMin(90,m_input.height()/4));
    m_pattern=m_input.copy((m_input.width()-w)/2,(m_input.height()-h)/2,w,h);emit inputsChanged();
}
void VisionRunner::clearHistory(){m_history.clear();emit changed();}
bool VisionRunner::save(const QUrl &url){
    if(m_busy||running()||!resultCurrent()||!url.isLocalFile())return false;
    const auto image=m_result;const auto path=url.toLocalFile();m_busy=true;m_status="导出中…";emit changed();
    m_watcher.setFuture(QtConcurrent::run([image,path]{
        Job job;job.saved=true;QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly)||!image.save(&file,"PNG")||!file.commit())job.error="PNG保存失败";
        return job;
    }));return true;
}
VisionRunner::Job VisionRunner::process(const QImage &input,const QImage &pattern,const QString &mode,const QVariantMap &options,quint64 revision){
    Job job;job.source=input;job.revision=revision;QElapsedTimer timer;timer.start();
    try {
        const auto mat=ScreenCapture::qImageToMat(input);
        LabAlgorithms::Result result;
        if(mode=="match")result=LabAlgorithms::match(mat,ScreenCapture::qImageToMat(pattern),options.value("threshold",0.8).toDouble());
        else if(mode=="stream")result=LabAlgorithms::stream(mat,options);
        else if(mode=="graph") {
            const auto nodes=options.value("nodes").toList(),edges=options.value("edges").toList();
            if(nodes.isEmpty()||nodes.size()>24)throw std::runtime_error("节点数须为1–24");
            QMap<int,QVariantMap> specs;QMap<int,int> parent;QSet<int> done;QMap<int,cv::Mat> outputs;
            for(const auto &value:nodes){auto n=value.toMap();int id=n.value("id").toInt();if(specs.contains(id))throw std::runtime_error("节点编号重复");specs[id]=n;}
            for(const auto &value:edges){auto e=value.toMap();int from=e.value("from").toInt(),to=e.value("to").toInt();
                if(!specs.contains(from)||!specs.contains(to)||parent.contains(to))throw std::runtime_error("连线不存在或同一节点有多个输入");parent[to]=from;}
            for(auto it=specs.cbegin();it!=specs.cend();++it){
                const auto type=it.value().value("type").toString();const bool source=type=="sample"||type=="screen";
                if(source==parent.contains(it.key()))throw std::runtime_error("源节点不能接入，运算节点必须连接输入");
            }
            while(done.size()<specs.size()){
                bool advanced=false;
                for(auto it=specs.cbegin();it!=specs.cend();++it){int id=it.key();if(done.contains(id)||(parent.contains(id)&&!done.contains(parent[id])))continue;
                    const auto n=it.value();const QString type=n.value("type").toString();
                    const auto screen=options.value("screens").toMap().value(QString::number(id)).value<QImage>();
                    QElapsedTimer nodeTime;nodeTime.start();
                    auto r=LabAlgorithms::node(parent.contains(id)?outputs[parent[id]]:(type=="screen"?ScreenCapture::qImageToMat(screen):mat),type,n.value("params").toMap());
                    r.values["ms"]=nodeTime.nsecsElapsed()/1e6;
                    if(!r.error.isEmpty())throw std::runtime_error(r.error.toStdString());
                    outputs[id]=r.image;job.nodes[id]=ScreenCapture::matToQImage(r.image);done.insert(id);advanced=true;
                    job.summary[QString::number(id)]=r.values;
                }
                if(!advanced)throw std::runtime_error("存在循环连线");
            }
            const int output=options.value("output",nodes.last().toMap().value("id")).toInt();
            if(!outputs.contains(output))throw std::runtime_error("请选择有效输出节点");
            result.image=outputs[output];result.values=job.summary;result.values["output"]=output;
        } else throw std::runtime_error("未知运行模式");
        job.error=result.error;job.image=ScreenCapture::matToQImage(result.image);job.summary=result.values;
        if(job.error.isEmpty()&&job.image.isNull())job.error="没有可显示的图像";
    } catch(const cv::Exception &e){job.error=QString("OpenCV：%1").arg(QString::fromUtf8(e.what()));}
      catch(const std::exception &e){job.error=QString::fromUtf8(e.what());}
    job.ms=timer.nsecsElapsed()/1e6;return job;
}
