#pragma once

#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <opencv2/core.hpp>
#include <vector>

// 「QtConcurrent 与 QFuture」的演示：检测同一批工件图，
// 一种是直接在界面线程里一张张做（界面会卡住），一种是交给线程池并行做（界面照常响应）。
class ConcurrencyDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString lastRun READ lastRun NOTIFY changed)
    Q_PROPERTY(double blockingMs READ blockingMs NOTIFY changed)
    Q_PROPERTY(double poolMs READ poolMs NOTIFY changed)
    Q_PROPERTY(int threads READ threads CONSTANT)
    Q_PROPERTY(int count MEMBER m_count NOTIFY changed)

public:
    explicit ConcurrencyDemo(QObject *parent = nullptr);

    bool running() const { return m_watcher.isRunning(); }
    QString lastRun() const { return m_lastRun; }
    double blockingMs() const { return m_blockingMs; }
    double poolMs() const { return m_poolMs; }
    int threads() const;

    Q_INVOKABLE void runBlocking();   // 在调用者（界面）线程里串行做完才返回
    Q_INVOKABLE void runPool();       // 立即返回，线程池里并行做，做完发 changed

signals:
    void changed();

private:
    void prepare();
    std::vector<cv::Mat> m_images;
    QFutureWatcher<int> m_watcher;
    QElapsedTimer m_clock;
    QString m_lastRun;
    double m_blockingMs = 0;
    double m_poolMs = 0;
    int m_count = 200;
};
