// 信号与槽：四种连接方式的执行顺序
//
// 运行：./example_qt_signals_slots
// 程序依次用 Direct / Queued / BlockingQueued / Auto 四种方式，从工作线程向主线程发信号，
// 打印每一步发生在哪个线程，观察「emit 返回」和「槽执行」谁先谁后。

#include <QCoreApplication>
#include <QMutex>
#include <QThread>
#include <QTimer>
#include <cstdio>

// 打印一行，并标出当前在哪个线程。两个线程都会调用，加锁避免输出交错。
static void say(const QString &what)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);
    const bool onMain = QThread::currentThread() == QCoreApplication::instance()->thread();
    std::printf("%s %s\n", onMain ? "[主线程]  " : "[工作线程]", qPrintable(what));
    std::fflush(stdout);
}

// [region declare]
// 发送者：声明信号，只写声明，不写实现——实现由 moc 生成
class Sensor : public QObject {
    Q_OBJECT
signals:
    void measured(int value);
};

// 接收者：槽就是普通的成员函数
class Display : public QObject {
    Q_OBJECT
public slots:
    void show(int value) { say(QStringLiteral("槽 show(%1)").arg(value)); }
};
// [endregion]

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Display display;            // 属于主线程
    Sensor sensor;
    QThread worker;
    sensor.moveToThread(&worker); // sensor 从此属于工作线程
    worker.start();

    const QList<QPair<Qt::ConnectionType, const char *>> types = {
        {Qt::DirectConnection, "DirectConnection"},
        {Qt::QueuedConnection, "QueuedConnection"},
        {Qt::BlockingQueuedConnection, "BlockingQueuedConnection"},
        {Qt::AutoConnection, "AutoConnection"},
    };

    int round = 0;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &app, [&] {
        if (round == types.size()) {
            worker.quit();
            worker.wait();
            app.quit();
            return;
        }
        const auto [type, name] = types[round++];
        std::printf("\n==== %s ====\n", name);

        // [region connect]
        QObject::disconnect(&sensor, nullptr, &display, nullptr);
        QObject::connect(&sensor, &Sensor::measured,  // 谁发出、哪个信号
                         &display, &Display::show,    // 谁接收、哪个槽
                         type);                       // 连接方式，默认是 AutoConnection
        // [endregion]

        // [region emit]
        // 让 emit 发生在工作线程里
        QMetaObject::invokeMethod(&sensor, [&sensor] {
            say(QStringLiteral("emit measured(42)"));
            emit sensor.measured(42);
            say(QStringLiteral("emit 返回"));
        });
        // [endregion]
    });
    timer.start(300);

    return app.exec();
}

#include "main.moc"
