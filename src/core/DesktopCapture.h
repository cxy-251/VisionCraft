#pragma once

#include <QImage>
#include <QMediaCaptureSession>
#include <QScreenCapture>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

// GUI线程拥有采集会话；Wayland通过系统屏幕共享选择器授权。
// 只保留最新视频帧，按处理节奏转为QImage，不为每帧建立处理队列。
class DesktopCapture : public QObject {
    Q_OBJECT
public:
    explicit DesktopCapture(QObject *parent=nullptr);
    ~DesktopCapture() override;
    void start();
    void stop();
    bool active() const { return m_active; }
    bool ready() const { return m_frame.isValid(); }
    QImage image(const QString &kind=QStringLiteral("full")) const;
    static bool isWaylandSession();
signals:
    void frameAvailable();
    void failed(const QString &message);
private:
    void fail(const QString &message);
    QMediaCaptureSession m_session;
    QScreenCapture m_capture;
    QVideoSink m_sink;
    QVideoFrame m_frame;
    QTimer m_timeout;
    bool m_active=false;
};
