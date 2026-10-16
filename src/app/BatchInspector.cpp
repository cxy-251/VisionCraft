#include "BatchInspector.h"
#include "LabAlgorithms.h"
#include "core/ScreenCapture.h"
#include <QtConcurrent/QtConcurrentRun>
#include <QImageReader>
#include <QDir>
#include <QFileInfo>
#include <QThreadPool>
#include <QThread>
#include <QSaveFile>

BatchInspector::BatchInspector(QObject *parent):QObject(parent){
    connect(this,&BatchInspector::settingsChanged,this,[this]{++m_revision;emit changed();});
    m_poll.setInterval(150);connect(&m_poll,&QTimer::timeout,this,&BatchInspector::changed);
    connect(&m_watcher,&QFutureWatcher<Job>::finished,this,[this]{
        const auto job=m_watcher.result();m_busy=false;m_poll.stop();
        if(job.error.isEmpty()){
            m_records=job.records;m_selected=m_records.isEmpty()?-1:0;m_resultRevision=job.revision;
            emit recordsChanged();emit selectionChanged();
            m_status=QString("完成 %1 张，跳过 %2 张；最多60张 / 合计2400万像素").arg(m_records.size()).arg(job.skipped);
        }else m_status=job.error;
        emit changed();
    });
}
BatchInspector::~BatchInspector(){m_watcher.waitForFinished();}
QVariantList BatchInspector::rows() const{
    QVariantList result;for(int i=0;i<m_records.size();++i){const auto&r=m_records[i];
        result.append(QVariantMap{{"index",i},{"name",r.name},{"passed",r.passed},{"diameter",r.diameter},{"area",r.area},{"ms",r.ms},{"reason",r.reason},{"worker",r.worker}});
    }return result;
}
QVariantMap BatchInspector::summary() const{int passed=0;double ms=0;for(const auto&r:m_records){passed+=r.passed;ms+=r.ms;}
    return {{"total",m_records.size()},{"passed",passed},{"ng",m_records.size()-passed},{"yield",m_records.isEmpty()?0:100.0*passed/m_records.size()},{"meanMs",m_records.isEmpty()?0:ms/m_records.size()}};
}
QImage BatchInspector::selectedImage()const{return m_selected>=0&&m_selected<m_records.size()?m_records[m_selected].image:QImage{};}
void BatchInspector::select(int index){if(index>=0&&index<m_records.size()){m_selected=index;emit selectionChanged();}}
void BatchInspector::generate(){launch({},true);}
void BatchInspector::loadFolder(const QUrl &url){if(url.isLocalFile())launch(url.toLocalFile(),false);}
void BatchInspector::rerun(){if(!m_records.isEmpty())launch({},false);}
void BatchInspector::launch(const QString &folder,bool sample){
    if(m_busy)return;
    m_busy=true;*m_progress=0;m_status="加载并检测…";m_poll.start();emit changed();
    const int threads=qBound(1,m_threads,8);const double tolerance=qBound(0.0,m_tolerance,50.0);
    const auto old=m_records;const auto progress=m_progress;const auto revision=m_revision;
    m_watcher.setFuture(QtConcurrent::run([folder,sample,threads,tolerance,old,progress,revision]{
        Job job;job.revision=revision;
        try{
            if(sample){for(int i=0;i<24;++i){Record r;r.original=ScreenCapture::matToQImage(LabAlgorithms::mountingSample(i));r.name=QString("part_%1.png").arg(i+1,2,10,QChar('0'));job.records.append(r);}}
            else if(folder.isEmpty())job.records=old;
            else{
                const QDir dir(folder);if(!dir.exists()){job.error="文件夹不存在";return job;}
                const auto names=dir.entryList(QDir::Files,QDir::Name);qint64 pixels=0;
                const auto formats=QImageReader::supportedImageFormats();
                for(const auto &name:names){
                    if(!formats.contains(QFileInfo(name).suffix().toLower().toLatin1()))continue;
                    if(job.records.size()>=60){++job.skipped;continue;}
                    QImageReader reader(dir.filePath(name));reader.setAutoTransform(true);const auto size=reader.size();
                    const qint64 count=qint64(size.width())*size.height();
                    if(QFileInfo(dir.filePath(name)).size()>128*1024*1024||!size.isValid()||count>4000000||pixels+count>24000000){++job.skipped;continue;}
                    auto image=reader.read();if(image.isNull()){++job.skipped;continue;}
                    pixels+=count;Record r;r.name=name;r.original=image;job.records.append(r);
                }
            }
            if(job.records.isEmpty()){job.error="没有可处理的图片；支持Qt图片格式，单张最多400万像素";return job;}
            QThreadPool pool;pool.setMaxThreadCount(threads);QList<QFuture<Record>> futures;
            for(const auto &record:job.records)futures.append(QtConcurrent::run(&pool,[record,tolerance,progress]{
                Record r=record;
                try{const auto result=LabAlgorithms::inspectMounting(ScreenCapture::qImageToMat(r.original),tolerance);
                    r.image=ScreenCapture::matToQImage(result.image);r.passed=result.passed;r.diameter=result.diameter;r.area=result.area;r.ms=result.ms;r.reason=result.reason;
                }catch(const cv::Exception&e){r.passed=false;r.reason=QString::fromUtf8(e.what());r.image=r.original;}
                r.worker=QString::number(quintptr(QThread::currentThreadId()),16);++*progress;return r;
            }));
            job.records.clear();for(auto &future:futures)job.records.append(future.result());
        }catch(const std::exception&e){job.error=QString::fromUtf8(e.what());}
        return job;
    }));
}
bool BatchInspector::exportCsv(const QUrl &url){
    if(!current()||m_busy||!url.isLocalFile())return false;
    auto quote=[](QString text){text.replace('"',"\"\"");return '"'+text+'"';};
    QByteArray data="\xEF\xBB\xBFname,result,diameter_px,defect_area_px,ms,worker,reason\r\n";
    for(const auto&r:m_records)data+=(QStringList{quote(r.name),r.passed?"PASS":"NG",QString::number(r.diameter,'f',2),QString::number(r.area,'f',2),QString::number(r.ms,'f',2),quote(r.worker),quote(r.reason)}.join(',')+"\r\n").toUtf8();
    QSaveFile file(url.toLocalFile());const bool ok=file.open(QIODevice::WriteOnly)&&file.write(data)==data.size()&&file.commit();
    m_status=ok?"CSV已导出（全部行）":"CSV导出失败";emit changed();return ok;
}
