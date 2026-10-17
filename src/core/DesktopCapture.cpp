#include "DesktopCapture.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

DesktopCapture::DesktopCapture(QObject *parent):QObject(parent)
{
    m_session.setScreenCapture(&m_capture);
    m_session.setVideoSink(&m_sink);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(60000);
    connect(&m_timeout,&QTimer::timeout,this,[this] {
        fail(tr("等待屏幕共享超时；请重新取图并在系统窗口中选择屏幕"));
    });
    connect(&m_capture,&QScreenCapture::errorOccurred,this,[this](QScreenCapture::Error error,const QString &text) {
        if(m_active && error!=QScreenCapture::NoError)
            fail(tr("屏幕共享未成功：%1").arg(text));
    });
    connect(&m_capture,&QScreenCapture::activeChanged,this,[this](bool active) {
        if(!active && m_active) fail(tr("屏幕共享已结束或被取消"));
    });
    connect(&m_sink,&QVideoSink::videoFrameChanged,this,[this](const QVideoFrame &frame) {
        if(!m_active || !frame.isValid()) return;
        m_frame=frame;
        m_timeout.stop();
        emit frameAvailable();
    });
}
DesktopCapture::~DesktopCapture() { stop(); }
bool DesktopCapture::isWaylandSession()
{
    return QGuiApplication::platformName().startsWith(QStringLiteral("wayland")) ||
        qEnvironmentVariable("XDG_SESSION_TYPE")==QStringLiteral("wayland");
}
void DesktopCapture::start()
{
    if(m_active) return;
    m_frame={};
    m_active=true;
    // Wayland由portal选择屏幕，QScreen无法指定实际共享源。
    if(!isWaylandSession()) m_capture.setScreen(QGuiApplication::primaryScreen());
    m_timeout.start();
    m_capture.start();
}
void DesktopCapture::stop()
{
    m_active=false;
    m_timeout.stop();
    m_capture.stop();
    m_frame={};
}
void DesktopCapture::fail(const QString &message)
{
    stop();
    emit failed(message);
}
QImage DesktopCapture::image(const QString &kind) const
{
    QImage image=m_frame.toImage();
    if(image.isNull()) return {};
    if(kind=="full") return image;
    const QSize region=QSize(kind=="mouse"?640:800,kind=="mouse"?400:500).boundedTo(image.size());
    QPoint center(image.width()/2,image.height()/2);
    // Wayland不提供可信的全局指针/共享屏幕映射，调用方应禁止鼠标跟随。
    if(kind=="mouse" && !isWaylandSession()) {
        if(auto *screen=QGuiApplication::primaryScreen()) {
            const QPoint local=QCursor::pos()-screen->geometry().topLeft();
            center={qRound(local.x()*double(image.width())/screen->size().width()),
                    qRound(local.y()*double(image.height())/screen->size().height())};
        }
    }
    const int x=qBound(0,center.x()-region.width()/2,image.width()-region.width());
    const int y=qBound(0,center.y()-region.height()/2,image.height()-region.height());
    return image.copy(x,y,region.width(),region.height());
}
