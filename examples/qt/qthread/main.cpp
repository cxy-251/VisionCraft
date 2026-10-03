// QThread 的两种用法：继承重写 run()，或者把工作对象 moveToThread
//
// 运行：./example_qt_qthread     输出见 output.txt
// 每一步都打印「这行代码在哪个线程执行」。

#include <QCoreApplication>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <cstdio>

static QThread *g_main;

static void say(const char *what)
{
    const char *where = QThread::currentThread() == g_main ? "主线程" : "工作线程";
    std::printf("  [%s] %s\n", where, what);
    std::fflush(stdout);
}

// [region subclass]
// 用法一：继承 QThread，把要做的事写在 run() 里。run() 在新线程执行，跑完线程就结束
class Measure : public QThread {
    Q_OBJECT
public:
    void run() override
    {
        say("run()：开始测量");
        msleep(100);
        say("run()：测完了，发出 done");
        emit done(42);
    }
public slots:
    void stopEarly() { say("stopEarly() 槽"); }   // 这个槽在哪个线程执行？
signals:
    void done(int value);
};
// [endregion]

// [region worker]
// 用法二：普通的 QObject 写工作，移到一个 QThread 里。线程运行着事件循环，可以反复接收任务
class Worker : public QObject {
    Q_OBJECT
public slots:
    void measure(int channel)
    {
        say(qPrintable(QStringLiteral("measure(%1)").arg(channel)));
        QThread::msleep(50);
        emit done(channel * 10);
    }
signals:
    void done(int value);
};
// [endregion]

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    g_main = QThread::currentThread();

    std::printf("==== 1. 继承 QThread ====\n");
    {
        // [region use-subclass]
        Measure m;
        QEventLoop loop;
        QObject::connect(&m, &Measure::done, [](int v) { say(qPrintable(QStringLiteral("收到 done(%1)").arg(v))); });
        QObject::connect(&m, &QThread::finished, &loop, &QEventLoop::quit);
        m.start();
        QMetaObject::invokeMethod(&m, &Measure::stopEarly, Qt::QueuedConnection);
        loop.exec();
        // [endregion]
    }

    std::printf("\n==== 2. 工作对象 + moveToThread ====\n");
    {
        // [region use-worker]
        QThread thread;
        auto *worker = new Worker;
        worker->moveToThread(&thread);
        QObject::connect(&thread, &QThread::finished, worker, &QObject::deleteLater);   // 线程结束时删掉 worker

        QEventLoop loop;
        int received = 0;
        QObject::connect(worker, &Worker::done, &loop, [&](int v) {
            say(qPrintable(QStringLiteral("收到 done(%1)").arg(v)));
            if (++received == 3) loop.quit();
        });
        thread.start();
        for (int ch = 1; ch <= 3; ++ch)       // 跨线程调用：排进工作线程的事件队列，依次执行
            QMetaObject::invokeMethod(worker, [worker, ch] { worker->measure(ch); }, Qt::QueuedConnection);
        say("三个任务都发出去了，主线程继续做自己的事");
        loop.exec();

        thread.quit();                        // 让工作线程的事件循环退出
        thread.wait();                        // 等它真正结束，再离开作用域销毁 QThread
        // [endregion]
    }
    return 0;
}

#include "main.moc"
