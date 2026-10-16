#include "ImageWorkshop.h"
#include "PartGenerator.h"
#include "core/ScreenCapture.h"
#include <QBuffer>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImageReader>
#include <QSaveFile>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>

namespace {
constexpr qint64 MaxPixels=24*1024*1024;
constexpr qint64 MaxFileBytes=128*1024*1024;
bool allowedSize(const QSize &size) { return size.isValid() && qint64(size.width())*size.height()<=MaxPixels; }
QString operationName(const QString &name)
{
    if(name=="gray") return QStringLiteral("灰度化");
    if(name=="blur") return QStringLiteral("高斯模糊");
    if(name=="threshold") return QStringLiteral("固定阈值二值化");
    return QStringLiteral("Canny边缘");
}
QString describe(const QString &name, const ImageOperations::Params &p)
{
    if(name=="blur") return QStringLiteral("高斯模糊 · 核 %1×%1").arg(p.kernelSize);
    if(name=="threshold") return QStringLiteral("固定阈值二值化 · 阈值 %1").arg(p.threshold);
    if(name=="canny") return QStringLiteral("Canny · 低阈值 %1 · 高阈值 %2").arg(p.lowThreshold).arg(p.highThreshold);
    return QStringLiteral("灰度化");
}
}
ImageWorkshop::ImageWorkshop(QObject *parent):QObject(parent)
{
    m_preview.setSingleShot(true); m_preview.setInterval(180);
    connect(&m_preview,&QTimer::timeout,this,&ImageWorkshop::process);
    connect(&m_watcher,&QFutureWatcher<Job>::finished,this,[this] {
        const auto job=m_watcher.result();
        m_busy=false;
        if(m_kind==Kind::Save) {
            message(job.error.isEmpty() ? tr("已导出到电脑：%1").arg(job.source) : job.error);
        } else if(job.revision==m_revision) {
            if(!job.error.isEmpty()) message(job.error);
            else if(m_kind==Kind::Load) adoptInput(job.image,job.source);
            else {
                m_result=job.image; m_resultRevision=job.revision; m_resultDescription=job.description;
                emit imagesChanged(); message(tr("处理完成 · %1").arg(job.description));
            }
        } else message(tr("参数或图像已变化，旧任务结果未采用"));
        emit busyChanged();
        if(m_pendingPreview && hasInput()) { m_pendingPreview=false; m_preview.start(); }
    });
}
ImageWorkshop::~ImageWorkshop() { m_preview.stop(); m_watcher.waitForFinished(); }
void ImageWorkshop::message(const QString &text) { m_status=text; emit statusChanged(); }
// [region job]
void ImageWorkshop::start(Kind kind, std::function<Job()> task)
{
    m_preview.stop(); m_kind=kind; m_busy=true; emit busyChanged();
    const auto revision=m_revision;
    m_watcher.setFuture(QtConcurrent::run([task=std::move(task),revision] {
        Job result;
        try { result=task(); }
        catch(const std::exception &e) { result.error=QString::fromUtf8(e.what()); }
        catch(...) { result.error=QStringLiteral("图像任务执行失败"); }
        result.revision=revision; return result;
    }));
}
void ImageWorkshop::queuePreview()
{
    if(!m_autoPreview || !hasInput() || resultCurrent()) return;
    if(m_busy) m_pendingPreview=true;
    else m_preview.start();
}
void ImageWorkshop::changedSettings()
{
    ++m_revision; emit settingsChanged(); emit imagesChanged(); queuePreview();
}
void ImageWorkshop::adoptInput(const QImage &image, const QString &source)
{
    ++m_revision; m_input=image; m_source=source;
    m_result={}; m_resultDescription.clear();
    emit imagesChanged(); message(tr("已打开 %1 · %2×%3").arg(source).arg(image.width()).arg(image.height()));
    queuePreview();
}
// [endregion]
QVariantMap ImageWorkshop::parameters() const
{
    return {{"kernelSize",m_params.kernelSize},{"threshold",m_params.threshold},
        {"lowThreshold",m_params.lowThreshold},{"highThreshold",m_params.highThreshold}};
}
void ImageWorkshop::setOperation(const QString &name)
{
    if(name==m_operation || (name!="gray" && name!="blur" && name!="threshold" && name!="canny")) return;
    m_operation=name; changedSettings();
}
bool ImageWorkshop::setParameter(const QString &key, double value)
{
    if(!std::isfinite(value)) return false;
    const auto current=parameters();
    if(!current.contains(key)) return false;
    if(key=="kernelSize") {
        if(value<1 || value>51 || std::floor(value)!=value || int(value)%2==0) return false;
        if(m_params.kernelSize==int(value)) return true;
        m_params.kernelSize=int(value);
    } else {
        if(value<0 || value>255) return false;
        if(current.value(key).toDouble()==value) return true;
        if(key=="threshold") m_params.threshold=value;
        else if(key=="lowThreshold") m_params.lowThreshold=value;
        else m_params.highThreshold=value;
    }
    changedSettings(); return true;
}
void ImageWorkshop::setAutoPreview(bool value)
{
    if(value==m_autoPreview) return;
    m_autoPreview=value; emit settingsChanged();
    if(value) queuePreview();
    else { m_preview.stop(); m_pendingPreview=false; }
}
// [region open]
bool ImageWorkshop::open(const QUrl &url)
{
    if(m_busy) { message(tr("请等待当前图像任务完成")); return false; }
    if(!url.isLocalFile()) { message(tr("请选择电脑上的图片文件")); return false; }
    m_pendingPreview=false; message(tr("正在打开图片…"));
    const QString path=url.toLocalFile();
    start(Kind::Load,[path] {
        Job job; job.source=QFileInfo(path).fileName();
        if(QFileInfo(path).size()>MaxFileBytes) { job.error=QStringLiteral("图片文件超过128 MiB限制"); return job; }
        QImageReader reader(path); reader.setAutoTransform(true);
        if(!allowedSize(reader.size())) { job.error=QStringLiteral("无法识别图片尺寸，或像素总数超过24 Mi像素"); return job; }
        const auto image=reader.read();
        if(image.isNull()) job.error=QStringLiteral("打开失败：%1").arg(reader.errorString());
        else if(!allowedSize(image.size())) job.error=QStringLiteral("图片像素总数超过24 Mi像素");
        else {
            job.image=image.convertToFormat(QImage::Format_RGB888);
            if(job.image.isNull()) job.error=QStringLiteral("图片转换失败，原有图片保留");
        }
        return job;
    });
    return true;
}
// [endregion]
void ImageWorkshop::useSample()
{
    if(m_busy) return;
    m_pendingPreview=false; message(tr("正在生成工件示例…"));
    start(Kind::Load,[] {
        PartGenerator generator(7); Job job;
        job.image=ScreenCapture::matToQImage(generator.make(VC_DEFECT_SCRATCH).image);
        job.source=QStringLiteral("模拟划痕工件 · seed 7"); return job;
    });
}
void ImageWorkshop::captureScreen()
{
    if(m_busy) return;
    // QScreen采集只在GUI线程、明确点击时调用；不用于自动截图或验证。
    const auto image=ScreenCapture::grabScreen();
    if(image.isNull()) { message(tr("当前桌面未返回屏幕图像，请打开本地图片或使用示例")); return; }
    if(!allowedSize(image.size())) { message(tr("屏幕图像超过24 Mi像素限制")); return; }
    adoptInput(image.convertToFormat(QImage::Format_RGB888),tr("屏幕单帧"));
}
// [region process]
void ImageWorkshop::process()
{
    if(!hasInput()) { message(tr("请先打开图片或使用示例")); return; }
    if(m_busy) { m_pendingPreview=true; return; }
    m_pendingPreview=false;
    const QImage input=m_input;
    const QString mode=m_operation;
    const auto params=m_params;
    message(tr("正在处理 · %1").arg(operationName(mode)));
    start(Kind::Process,[input,mode,params] {
        const cv::Mat mat=ScreenCapture::qImageToMat(input);
        const auto operation=mode=="gray" ? ImageOperations::Operation::Grayscale :
            mode=="blur" ? ImageOperations::Operation::GaussianBlur :
            mode=="threshold" ? ImageOperations::Operation::BinaryThreshold : ImageOperations::Operation::Canny;
        QElapsedTimer clock; clock.start();
        const auto result=ImageOperations::run(mat,operation,params);
        const double ms=clock.nsecsElapsed()/1e6;
        Job job; job.error=result.error;
        if(job.error.isEmpty()) {
            job.image=ScreenCapture::matToQImage(result.image);
            if(job.image.isNull()) { job.error=QStringLiteral("处理结果转换失败"); return job; }
            job.description=QStringLiteral("%1 · %2 ms · %3通道").arg(describe(mode,params)).arg(ms,0,'f',2).arg(result.image.channels());
        }
        return job;
    });
}
// [endregion]
// [region save]
bool ImageWorkshop::save(const QUrl &url)
{
    if(m_busy || !resultCurrent()) { message(tr("请先完成当前参数的处理，再导出结果")); return false; }
    if(!url.isLocalFile()) { message(tr("请选择电脑上的目标文件")); return false; }
    const QString path=url.toLocalFile();
    const auto suffix=QFileInfo(path).suffix().toLower();
    if(suffix!="png" && suffix!="jpg" && suffix!="jpeg") { message(tr("请使用.png、.jpg或.jpeg扩展名")); return false; }
    const QImage image=m_result;
    message(tr("正在导出结果…"));
    start(Kind::Save,[image,path,suffix] {
        Job job; job.source=QFileInfo(path).fileName();
        QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly);
        if(!image.save(&buffer,suffix=="png" ? "PNG" : "JPEG")) { job.error=QStringLiteral("结果图编码失败"); return job; }
        QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())
            job.error=QStringLiteral("导出失败：%1").arg(file.errorString());
        return job;
    });
    return true;
}
// [endregion]
void ImageWorkshop::clear()
{
    ++m_revision; m_preview.stop(); m_pendingPreview=false;
    m_input={}; m_result={}; m_source.clear(); m_resultDescription.clear();
    emit imagesChanged(); message(tr("已清空图像"));
}
