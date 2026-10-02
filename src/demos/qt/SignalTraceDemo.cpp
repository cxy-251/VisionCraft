#include "SignalTraceDemo.h"

#include <QCoreApplication>
#include <QMetaObject>

// 发出信号的对象。主线程一个，工作线程一个（moveToThread 之后它「属于」工作线程）。
class TraceEmitter : public QObject {
    Q_OBJECT
signals:
    void ping(int seq);
};

// 接收信号的对象，属于主线程
class TraceReceiver : public QObject {
    Q_OBJECT
public:
    explicit TraceReceiver(SignalTraceDemo *demo) : m_demo(demo) {}

public slots:
    void onPing(int seq)
    {
        m_demo->record(seq, QStringLiteral("slot"), QStringLiteral("槽 onPing(%1) 开始执行").arg(seq));
        if (const int ms = m_demo->m_slotDelayMs.load())
            QThread::msleep(ms); // 模拟耗时的槽
        m_demo->record(seq, QStringLiteral("slotEnd"), QStringLiteral("槽 onPing(%1) 执行完").arg(seq));
    }

private:
    SignalTraceDemo *m_demo;
};

SignalTraceDemo::SignalTraceDemo(QObject *parent)
    : QObject(parent)
    , m_mainEmitter(new TraceEmitter)
    , m_workerEmitter(new TraceEmitter)
    , m_receiver(new TraceReceiver(this))
{
    m_clock.start();
    m_worker.setObjectName(QStringLiteral("demo-worker"));
    m_workerEmitter->moveToThread(&m_worker);
    m_worker.start();
    reconnect();
}

SignalTraceDemo::~SignalTraceDemo()
{
    m_worker.quit();
    m_worker.wait();
    delete m_workerEmitter; // 线程已停，可以在这里删
    delete m_mainEmitter;
    delete m_receiver;
}

void SignalTraceDemo::setConnectionType(int type)
{
    if (m_type == type)
        return;
    m_type = type;
    reconnect();
    emit connectionTypeChanged();
}

void SignalTraceDemo::setSlotDelayMs(int ms)
{
    if (m_slotDelayMs == ms)
        return;
    m_slotDelayMs = ms;
    emit slotDelayMsChanged();
}

void SignalTraceDemo::reconnect()
{
    const auto type = static_cast<Qt::ConnectionType>(m_type);
    for (TraceEmitter *e : {m_mainEmitter, m_workerEmitter}) {
        QObject::disconnect(e, &TraceEmitter::ping, m_receiver, &TraceReceiver::onPing);
        QObject::connect(e, &TraceEmitter::ping, m_receiver, &TraceReceiver::onPing, type);
    }
}

void SignalTraceDemo::emitFromMain()
{
    const int seq = ++m_seq;
    {
        QMutexLocker lock(&m_mutex);
        m_roundStartNs = m_clock.nsecsElapsed();
    }
    if (m_type == BlockingQueued) {
        // 发送者和接收者都在主线程：主线程会等待一个只能由它自己处理的事件，永远等不到。
        // 真这么做界面会卡死，所以这里拒绝执行，原理见本节的「避坑」。
        record(seq, QStringLiteral("warn"),
               QStringLiteral("拒绝执行：同一线程里用 BlockingQueuedConnection 会死锁"));
        return;
    }
    record(seq, QStringLiteral("emit"), QStringLiteral("emit ping(%1)").arg(seq));
    emit m_mainEmitter->ping(seq);
    record(seq, QStringLiteral("return"), QStringLiteral("emit 返回"));
}

void SignalTraceDemo::emitFromWorker()
{
    const int seq = ++m_seq;
    {
        QMutexLocker lock(&m_mutex);
        m_roundStartNs = m_clock.nsecsElapsed();
    }
    // 把一段代码投递到工作线程执行：第三个参数指定在 m_workerEmitter 所属的线程里运行
    QMetaObject::invokeMethod(m_workerEmitter, [this, seq] {
        record(seq, QStringLiteral("emit"), QStringLiteral("emit ping(%1)").arg(seq));
        emit m_workerEmitter->ping(seq);
        record(seq, QStringLiteral("return"), QStringLiteral("emit 返回"));
    }, Qt::QueuedConnection);
}

void SignalTraceDemo::clear()
{
    {
        QMutexLocker lock(&m_mutex);
        m_events.clear();
    }
    emit eventsChanged();
}

QVariantList SignalTraceDemo::events() const
{
    QMutexLocker lock(&m_mutex);
    return m_events;
}

void SignalTraceDemo::record(int seq, const QString &kind, const QString &text)
{
    const bool onMain = QThread::currentThread() == QCoreApplication::instance()->thread();
    bool schedule = false;
    {
        QMutexLocker lock(&m_mutex);
        const double ms = (m_clock.nsecsElapsed() - m_roundStartNs) / 1e6;
        m_events.append(QVariantMap{
            {QStringLiteral("seq"), seq},
            {QStringLiteral("kind"), kind},
            {QStringLiteral("text"), text},
            {QStringLiteral("main"), onMain},
            {QStringLiteral("ms"), ms},
        });
        if (m_events.size() > 200)
            m_events.removeFirst();
        schedule = !m_publishPending;
        m_publishPending = true;
    }
    // [region publish]
    // 界面只能在主线程更新：不管谁调用 record，都排队到主线程去通知
    if (schedule)
        QMetaObject::invokeMethod(this, &SignalTraceDemo::publish, Qt::QueuedConnection);
    // [endregion]
}

void SignalTraceDemo::publish()
{
    {
        QMutexLocker lock(&m_mutex);
        m_publishPending = false;
    }
    emit eventsChanged();
}

#include "SignalTraceDemo.moc"
