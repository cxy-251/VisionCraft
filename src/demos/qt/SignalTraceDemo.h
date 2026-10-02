#pragma once

#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <atomic>

class TraceEmitter;
class TraceReceiver;

// 「信号与槽」一节的交互演示。
//
// 接收者固定在主线程（界面线程）。信号可以从主线程或工作线程发出，连接方式可切换。
// 每一步（emit 开始、槽开始、槽结束、emit 返回）都记录下发生的线程和相对时间，
// 界面按时间顺序画成两条泳道，用来对比同步与异步。
class SignalTraceDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int connectionType READ connectionType WRITE setConnectionType NOTIFY connectionTypeChanged)
    Q_PROPERTY(int slotDelayMs READ slotDelayMs WRITE setSlotDelayMs NOTIFY slotDelayMsChanged)
    Q_PROPERTY(QVariantList events READ events NOTIFY eventsChanged)

public:
    // 与 Qt::ConnectionType 的取值一致，方便界面直接用数字
    enum Type { Auto = Qt::AutoConnection, Direct = Qt::DirectConnection,
                Queued = Qt::QueuedConnection, BlockingQueued = Qt::BlockingQueuedConnection };
    Q_ENUM(Type)

    explicit SignalTraceDemo(QObject *parent = nullptr);
    ~SignalTraceDemo() override;

    int connectionType() const { return m_type; }
    void setConnectionType(int type);

    int slotDelayMs() const { return m_slotDelayMs; }
    void setSlotDelayMs(int ms);

    QVariantList events() const;

    Q_INVOKABLE void emitFromMain();
    Q_INVOKABLE void emitFromWorker();
    Q_INVOKABLE void clear();

signals:
    void connectionTypeChanged();
    void slotDelayMsChanged();
    void eventsChanged();

private:
    friend class TraceReceiver;
    void reconnect();
    void record(int seq, const QString &kind, const QString &text);
    void publish();

    TraceEmitter *m_mainEmitter = nullptr;
    TraceEmitter *m_workerEmitter = nullptr;
    TraceReceiver *m_receiver = nullptr;
    QThread m_worker;

    int m_type = Auto;
    std::atomic<int> m_slotDelayMs{0};
    std::atomic<int> m_seq{0};

    mutable QMutex m_mutex;          // 工作线程和主线程都会写日志
    QVariantList m_events;
    QElapsedTimer m_clock;
    qint64 m_roundStartNs = 0;
    bool m_publishPending = false;
};
