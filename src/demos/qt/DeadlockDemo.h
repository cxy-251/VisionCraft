#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

// 「避坑：同一线程里的 BlockingQueuedConnection」的真实演示。
//
// 在一个专门牺牲掉的线程里真的触发一次死锁：界面不受影响，但那个线程永远卡住，
// 只有进程退出才能回收。为此每次运行程序只允许触发一次。
class DeadlockDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString qtWarning READ qtWarning NOTIFY stateChanged)

public:
    enum State { Idle, Running, Deadlocked, AlreadyUsed };
    Q_ENUM(State)

    explicit DeadlockDemo(QObject *parent = nullptr);

    State state() const { return m_state; }
    QString qtWarning() const { return m_qtWarning; }

    Q_INVOKABLE void trigger();

signals:
    void stateChanged();

private:
    State m_state = Idle;
    QString m_qtWarning;
};
