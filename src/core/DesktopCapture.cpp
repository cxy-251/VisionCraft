#include "DesktopCapture.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>
#include <QPointer>

#ifdef Q_OS_LINUX
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusVariant>
namespace {
// Qt 6.9还会广播处理所有portal Response；每次只保留一个屏幕共享请求。
QPointer<DesktopCapture> portalOwner;
constexpr auto PortalService="org.freedesktop.portal.Desktop";
constexpr auto PortalRequest="org.freedesktop.portal.Request";
}
#endif

DesktopCapture::DesktopCapture(QObject *parent):QObject(parent)
{
    m_session.setVideoSink(&m_sink);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(60000);
    connect(&m_timeout,&QTimer::timeout,this,[this] {
        fail(tr("等待屏幕共享超时；请重新取图并在系统窗口中选择屏幕"));
    });
#ifdef Q_OS_LINUX
    m_portalListening=QDBusConnection::sessionBus().connect(PortalService,QString{},PortalRequest,"Response",
        this,SLOT(portalResponse(uint,QVariantMap,QDBusMessage)));
#endif
    connect(&m_sink,&QVideoSink::videoFrameChanged,this,[this](const QVideoFrame &frame) {
        if(!m_active || !frame.isValid()) return;
#ifdef Q_OS_LINUX
        // X11等后端没有portal会话；真正的portal后端在首帧前已交付session_handle。
        m_waitingPortalSession=false;
#endif
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
#ifdef Q_OS_LINUX
    if(isWaylandSession()) {
        if(!m_portalListening) {
            emit failed(tr("无法监听系统屏幕共享会话，未开始取图"));
            return;
        }
        if(portalOwner) {
            emit failed(tr("上一次屏幕共享尚未结束，请先停止或取消后再取图"));
            return;
        }
        portalOwner=this;
        m_waitingPortalSession=true;
    }
#endif
    m_frame={};
    m_sink.setVideoFrame({});
    m_active=true;
    m_capture=std::make_unique<QScreenCapture>();
    m_session.setScreenCapture(m_capture.get());
    connect(m_capture.get(),&QScreenCapture::errorOccurred,this,[this](QScreenCapture::Error error,const QString &text) {
        if(m_active && error!=QScreenCapture::NoError) {
#ifdef Q_OS_LINUX
            // CreateSession调用本身失败时没有可等待的会话应答。
            if(m_portalSession.isEmpty()) m_waitingPortalSession=false;
#endif
            fail(tr("屏幕共享未成功：%1").arg(text));
        }
    });
    connect(m_capture.get(),&QScreenCapture::activeChanged,this,[this](bool active) {
        if(!active && m_active) fail(tr("屏幕共享已结束或被取消"));
    });
    // Wayland由portal选择屏幕，QScreen无法指定实际共享源。
    if(!isWaylandSession()) m_capture->setScreen(QGuiApplication::primaryScreen());
    m_timeout.start();
    m_capture->start();
}
void DesktopCapture::stop()
{
    m_active=false;
    m_timeout.stop();
    m_frame={};
    m_sink.setVideoFrame({});
    if(m_capture) {
        // stop常由视频帧/错误回调触发，回到事件循环后再销毁后端。
        auto *capture=m_capture.release();
        capture->disconnect(this);
        capture->stop();
        m_session.setScreenCapture(nullptr);
        capture->deleteLater();
    }
#ifdef Q_OS_LINUX
    closePortalSession();
#endif
}
#ifdef Q_OS_LINUX
void DesktopCapture::portalResponse(uint response,const QVariantMap &results,const QDBusMessage &message)
{
    if(portalOwner!=this || !m_waitingPortalSession) return;
    auto sender=QDBusConnection::sessionBus().baseService();
    sender.remove(0,1); sender.replace('.','_');
    // 只接收本进程的CreateSession结果，不关闭其他应用的共享。
    const QString requestPrefix="/org/freedesktop/portal/desktop/request/"+sender+"/";
    if(!message.path().startsWith(requestPrefix)) return;
    if(response!=0) {
        m_waitingPortalSession=false;
        fail(tr("屏幕共享请求已被取消或拒绝"));
        return;
    }
    if(!results.contains("session_handle")) return;
    auto handle=results.value("session_handle");
    if(handle.metaType()==QMetaType::fromType<QDBusVariant>()) handle=handle.value<QDBusVariant>().variant();
    const QString path=handle.metaType()==QMetaType::fromType<QDBusObjectPath>()
        ?handle.value<QDBusObjectPath>().path():handle.toString();
    if(!path.startsWith("/org/freedesktop/portal/desktop/session/"+sender+"/")) return;
    m_portalSession=path;
    m_waitingPortalSession=false;
    if(!m_active) closePortalSession(); // 取消发生在CreateSession应答之前。
}
void DesktopCapture::closePortalSession()
{
    if(!m_portalSession.isEmpty()) {
        const auto message=QDBusMessage::createMethodCall(PortalService,m_portalSession,
            "org.freedesktop.portal.Session","Close");
        QDBusConnection::sessionBus().asyncCall(message);
        m_portalSession.clear();
    }
    if(portalOwner==this && !m_waitingPortalSession) portalOwner.clear();
}
#endif
void DesktopCapture::fail(const QString &message)
{
#ifdef Q_OS_LINUX
    if(m_portalSession.isEmpty()) m_waitingPortalSession=false;
#endif
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
