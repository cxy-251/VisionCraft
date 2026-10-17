#include "VisionRunner.h"
#include "LabAlgorithms.h"
#include "core/ScreenCapture.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QElapsedTimer>
#include <QSaveFile>
#include <QDateTime>
#include <QSet>
#include <stdexcept>

VisionRunner::VisionRunner(QObject *parent):QObject(parent) {
    connect(&m_capture,&DesktopCapture::frameAvailable,this,[this] {
        if(m_waitingCapture) { m_waitingCapture=false; run(); }
    });
    connect(&m_capture,&DesktopCapture::failed,this,[this](const QString &error) {
        stop(); ++m_revision; m_status=error; emit changed();
    });
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
        } else if(running()) m_status="输入或参数已变更，等待下一帧";
        else if(m_status=="处理中…") m_status="输入或参数已变更，请重新运行";
        emit changed();
    });
}
VisionRunner::~VisionRunner(){m_timer.stop(); m_capture.stop(); m_watcher.waitForFinished();}
void VisionRunner::run() {
    if(m_busy || m_waitingCapture) return;
    QImage input=m_input;
    auto options=m_options;
    const auto source=options.value("capture","input").toString();
    const auto nodes=options.value("nodes").toList();
    bool needsScreen=source!="input";
    if(m_mode=="graph") for(const auto &value:nodes)
        needsScreen=needsScreen || value.toMap().value("type").toString()=="screen";
    if(needsScreen && !m_capture.ready()) {
        m_waitingCapture=true;
        m_status="等待屏幕共享；请在系统窗口中选择屏幕，也可点击停止取消";
        emit changed();
        m_capture.start();
        return;
    }
    auto screenImage=[this](const QString &kind) {
        if(kind=="mouse" && waylandCapture()) return QImage{};
        QImage image=m_capture.image(kind);
        if(kind=="full" && (image.width()>1280 || image.height()>720))
            image=image.scaled(1280,720,Qt::KeepAspectRatio,Qt::FastTransformation);
        return image;
    };
    bool failed=false;
    if(source!="input") { input=screenImage(source); failed=input.isNull(); }
    if(m_mode=="graph") {
        QVariantMap screens; QMap<QString,QImage> snapshots;
        if(nodes.size()<=24) for(const auto &value:nodes) {
            const auto node=value.toMap(); if(node.value("type").toString()!="screen") continue;
            const auto kind=node.value("params").toMap().value("mode","center").toString();
            if(!snapshots.contains(kind)) snapshots[kind]=screenImage(kind);
            failed=failed || snapshots[kind].isNull();
            screens[QString::number(node.value("id").toInt())]=snapshots[kind];
        }
        options["screens"]=screens;
    }
    if(failed) {
        stop(); ++m_revision;
        m_status=waylandCapture()?"共享画面不可读取，或选择了Wayland不支持的鼠标跟随；请改用中心/全屏":"屏幕共享没有返回可读取的图像";
        emit changed(); return;
    }
    if(!running() || !needsScreen) m_capture.stop();
    if(running() && !m_streamTime.isValid()) m_streamTime.start();
    m_busy=true; m_status="处理中…"; emit changed();
    m_watcher.setFuture(QtConcurrent::run(&VisionRunner::process,input,m_pattern,m_mode,options,m_revision));
}
void VisionRunner::start(int fps) {
    if(m_timer.isActive() || busy()) return;
    m_streamTime.invalidate(); m_frameCount=0;
    m_timer.start(1000/qBound(1,fps,60)); emit changed(); run();
}
void VisionRunner::stop() {
    const bool active=running() || m_waitingCapture || m_capture.active();
    const bool wasCurrent=resultCurrent();
    m_timer.stop(); m_waitingCapture=false; m_capture.stop();
    if(!active) return;
    if(m_busy) { ++m_revision; if(wasCurrent)m_resultRevision=m_revision; }
    m_status="处理已停止，屏幕共享已释放"; emit changed();
}
void VisionRunner::centerTemplate(){
    if(m_input.isNull())return;
    const int w=qMax(1,qMin(160,m_input.width()/4)),h=qMax(1,qMin(90,m_input.height()/4));
    m_pattern=m_input.copy((m_input.width()-w)/2,(m_input.height()-h)/2,w,h);emit inputsChanged();
}
void VisionRunner::clearHistory(){m_history.clear();emit changed();}
bool VisionRunner::save(const QUrl &url){
    if(busy()||running()||!resultCurrent()||!url.isLocalFile())return false;
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
