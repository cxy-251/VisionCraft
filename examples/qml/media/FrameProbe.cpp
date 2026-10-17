#include "FrameProbe.h"
#include "core/ScreenCapture.h"
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QtConcurrent/QtConcurrentRun>
#include <opencv2/imgproc.hpp>

FrameProbe::FrameProbe(QObject *parent):QObject(parent) {
    // [region publish]
    connect(&m_watcher,&QFutureWatcher<Result>::finished,this,[this] {
        const auto result=m_watcher.result(); m_busy=false;
        if(result.epoch==m_epoch && m_enabled) {
            if(result.error.isEmpty()) {
                m_image=result.image; ++m_completed;
                m_info["analyzedTimestampUs"]=result.timestamp;
                m_status=QString("已完成%1帧；忙时/节流跳过%2帧").arg(m_completed).arg(m_skipped);
            } else m_status=result.error;
        }
        emit changed();
    });
    // [endregion]
}
FrameProbe::~FrameProbe() { disconnect(m_connection); m_watcher.waitForFinished(); }
void FrameProbe::setSink(QVideoSink *sink) {
    if(sink==m_sink) return;
    disconnect(m_connection); m_sink=sink; reset();
    const auto generation=++m_sinkEpoch;
    if(sink) m_connection=connect(sink,&QVideoSink::videoFrameChanged,this,[this,generation](const QVideoFrame &frame) {
        if(generation==m_sinkEpoch) receive(frame);
    });
    emit sinkChanged();
}
void FrameProbe::setEnabled(bool value) {
    if(value==m_enabled) return;
    m_enabled=value; reset();
}
void FrameProbe::reset() {
    ++m_epoch; m_interval.invalidate(); m_image={}; m_info={};
    m_received=0; m_skipped=0; m_completed=0;
    m_status=m_enabled?"等待下一帧；旧分析结果已失效":"分析关闭；播放由MediaPlayer独立推进";
    emit changed();
}
// [region receive]
void FrameProbe::receive(const QVideoFrame &frame) {
    if(!frame.isValid()) return;
    ++m_received;
    m_info["received"]=m_received;
    m_info["timestampUs"]=frame.startTime();
    m_info["size"]=QString("%1×%2").arg(frame.width()).arg(frame.height());
    m_info["pixelFormat"]=QVideoFrameFormat::pixelFormatToString(frame.pixelFormat());
    // 视频仍正常播放；教学分析最多约5次/秒，不排队保存所有帧。
    if(!m_enabled || m_busy || (m_interval.isValid() && m_interval.elapsed()<200)) {
        if(m_enabled) ++m_skipped;
        emit changed(); return;
    }
    // 在接收者GUI线程完成Qt图像交付，工作线程只读取独立持有的QImage。
    const auto image=frame.toImage();
    if(image.isNull()) { m_status="当前帧不能转为CPU图像"; emit changed(); return; }
    m_interval.start(); m_busy=true;
    const auto epoch=m_epoch; const auto timestamp=frame.startTime();
    m_status="后台分析一帧…"; emit changed();
    m_watcher.setFuture(QtConcurrent::run([image,epoch,timestamp] {
        Result result; result.epoch=epoch; result.timestamp=timestamp;
        try {
            const auto bgr=ScreenCapture::qImageToMat(image);
            cv::Mat gray,edges; cv::cvtColor(bgr,gray,cv::COLOR_BGR2GRAY);
            cv::Canny(gray,edges,60,140);
            result.image=ScreenCapture::matToQImage(edges);
        } catch(const std::exception &error) { result.error=QString::fromUtf8(error.what()); }
        return result;
    }));
}
// [endregion]
