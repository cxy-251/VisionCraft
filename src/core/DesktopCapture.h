#pragma once

#include <QImage>
#include <QMediaCaptureSession>
#include <QScreenCapture>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>
#include <QVariantMap>
#include <memory>

#ifdef Q_OS_LINUX
#include <QDBusMessage>
#endif

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
#ifdef Q_OS_LINUX
    // Qt 6.9的PipeWire后端未关闭portal Session，应用显式归还自己的会话。
    Q_SLOT void portalResponse(uint response, const QVariantMap &results, const QDBusMessage &message);
    void closePortalSession();
    QString m_portalSession;
    bool m_waitingPortalSession=false;
    bool m_portalListening=false;
#endif
    QMediaCaptureSession m_session;
    std::unique_ptr<QScreenCapture> m_capture;
    QVideoSink m_sink;
    QVideoFrame m_frame;
    QTimer m_timeout;
    bool m_active=false;
};
