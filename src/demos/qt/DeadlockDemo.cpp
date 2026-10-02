#include "DeadlockDemo.h"

#include <QMutex>
#include <QPointer>
#include <QThread>
#include <QTimer>
#include <atomic>

namespace {

class Victim : public QObject {
    Q_OBJECT
signals:
    void request();
public slots:
    void handle() {} // 永远不会被执行到
};

std::atomic<bool> g_used{false};
std::atomic<bool> g_emitReturned{false};

// 截获 Qt 打印的那条「Dead lock detected」警告，展示给读者看
QtMessageHandler g_previousHandler = nullptr;
QMutex g_warningMutex;
QString g_warning;

void captureHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    if (msg.contains(QLatin1String("Dead lock detected"))) {
        QMutexLocker lock(&g_warningMutex);
        g_warning = msg;
    }
    if (g_previousHandler)
        g_previousHandler(type, ctx, msg);
}

} // namespace

DeadlockDemo::DeadlockDemo(QObject *parent)
    : QObject(parent)
    , m_state(g_used ? AlreadyUsed : Idle)
{
}

void DeadlockDemo::trigger()
{
    if (g_used.exchange(true)) {
        m_state = AlreadyUsed;
        emit stateChanged();
        return;
    }
    if (!g_previousHandler)
        g_previousHandler = qInstallMessageHandler(captureHandler);

    // 故意不设父对象、不销毁：线程卡死后无法正常结束，销毁一个仍在运行的 QThread 会直接让程序崩溃
    auto *thread = new QThread;
    thread->setObjectName(QStringLiteral("deadlock-victim"));
    auto *victim = new Victim;
    victim->moveToThread(thread);
    // 发送者和接收者都属于 thread，却用了「阻塞排队」连接
    connect(victim, &Victim::request, victim, &Victim::handle, Qt::BlockingQueuedConnection);
    thread->start();

    QMetaObject::invokeMethod(victim, [victim] {
        // [region deadlock]
        // 这里运行在 victim 所属的线程里。
        // BlockingQueuedConnection：把调用打包成事件，投递到「接收者所在线程」的事件队列，
        // 然后阻塞当前线程，直到那个事件被处理完。
        // 可接收者所在线程就是当前线程——它正卡在这里等，永远不会回到事件循环去处理那个事件。
        emit victim->request();
        // [endregion]
        g_emitReturned = true;
    }, Qt::QueuedConnection);

    m_state = Running;
    emit stateChanged();

    QPointer<DeadlockDemo> self(this);
    QTimer::singleShot(1500, this, [self] {
        if (!self)
            return;
        {
            QMutexLocker lock(&g_warningMutex);
            self->m_qtWarning = g_warning;
        }
        self->m_state = g_emitReturned ? Idle : Deadlocked;
        emit self->stateChanged();
    });
}

#include "DeadlockDemo.moc"
