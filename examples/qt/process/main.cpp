// QProcess：在程序里启动另一个程序，和它交换数据，结束它
//
// 运行：./example_qt_process     输出见 output.txt（只在 Linux 上写过：用到了 sh、sleep、head）
// 每个场景都用事件循环异步运行，打印 QProcess 发出的每个信号，看清它们的顺序。

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QProcess>
#include <QTimer>
#include <cstdio>

static void say(const QString &s)
{
    std::printf("  %s\n", qPrintable(s));
    std::fflush(stdout);
}

static QString statusName(QProcess::ExitStatus s) { return s == QProcess::NormalExit ? "NormalExit" : "CrashExit"; }

// 把 QProcess 的几个信号都打印出来；finished 或启动失败时退出局部事件循环
static void trace(QProcess &p, QEventLoop &loop, bool readOutput = true)
{
    QObject::connect(&p, &QProcess::started, [] { say("started"); });
    QObject::connect(&p, &QProcess::errorOccurred, [&loop](QProcess::ProcessError e) {
        say(QStringLiteral("errorOccurred(%1)").arg(e == QProcess::FailedToStart ? "FailedToStart" : e == QProcess::Crashed ? "Crashed" : QString::number(e)));
        if (e == QProcess::FailedToStart)
            loop.quit();   // 启动失败时不会有 finished
    });
    if (readOutput) {
        QObject::connect(&p, &QProcess::readyReadStandardOutput, [&p] { say("stdout: " + QString::fromUtf8(p.readAllStandardOutput()).trimmed()); });
        QObject::connect(&p, &QProcess::readyReadStandardError, [&p] { say("stderr: " + QString::fromUtf8(p.readAllStandardError()).trimmed()); });
    }
    QObject::connect(&p, &QProcess::finished, [&loop](int code, QProcess::ExitStatus st) {
        say(QStringLiteral("finished(exitCode=%1, %2)").arg(code).arg(statusName(st)));
        loop.quit();
    });
}

static void failedToStart()
{
    std::printf("==== 1. 程序不存在 ====\n");
    // [region failed]
    QProcess p;
    QEventLoop loop;
    trace(p, loop);
    p.start(QStringLiteral("no-such-program"), {});
    loop.exec();
    // [endregion]
}

static void normalRun()
{
    std::printf("\n==== 2. 正常运行：输出、错误输出、退出码 ====\n");
    // [region normal]
    QProcess p;
    QEventLoop loop;
    trace(p, loop);
    p.start(QStringLiteral("sh"), {QStringLiteral("-c"), QStringLiteral("echo 正常输出; echo 出错信息 >&2; exit 3")});
    loop.exec();
    // [endregion]
}

static void noShell()
{
    std::printf("\n==== 3. 参数原样传给程序，不经过 shell ====\n");
    // [region args]
    QProcess p;
    QEventLoop loop;
    trace(p, loop);
    p.start(QStringLiteral("echo"), {QStringLiteral("$HOME"), QStringLiteral("a b"), QStringLiteral("*.cpp")});
    loop.exec();
    // [endregion]
}

static void unreadOutput()
{
    std::printf("\n==== 4. 不读输出会怎样 ====\n");
    // [region unread]
    QProcess p;
    QEventLoop loop;
    trace(p, loop, false);                       // 不连接 readyRead，不读
    p.start(QStringLiteral("head"), {QStringLiteral("-c"), QStringLiteral("50000000"), QStringLiteral("/dev/zero")});
    loop.exec();
    say(QStringLiteral("子进程结束时，QProcess 里攒着 %1 字节没人读").arg(p.bytesAvailable()));
    // [endregion]
}

static void stop(const char *title, const QString &script)
{
    std::printf("\n==== %s ====\n", title);
    // [region stop]
    QProcess p;
    QEventLoop loop;
    trace(p, loop);
    p.start(QStringLiteral("sh"), {QStringLiteral("-c"), script});
    // started 只表示进程已经创建，不表示它已经执行到哪一行。等它自己说「准备好了」再发信号，
    // 否则 terminate 可能在 trap 那一行执行之前就到了
    p.waitForReadyRead();
    QElapsedTimer t;
    t.start();
    p.terminate();                               // 礼貌地请它退出（Linux 上是 SIGTERM）
    if (!p.waitForFinished(1000)) {              // 给它 1 秒收尾
        say(QStringLiteral("1 秒后还没退出，kill()"));
        p.kill();                                // 强制结束（SIGKILL），对方无法拒绝
        p.waitForFinished();
    }
    say(QStringLiteral("用时 %1 ms").arg(t.elapsed()));
    // [endregion]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    failedToStart();
    normalRun();
    noShell();
    unreadOutput();
    stop("5. terminate：普通程序收到就退出", QStringLiteral("echo 准备好了; sleep 30"));
    stop("6. terminate 被忽略：只好 kill", QStringLiteral("trap '' TERM; echo 准备好了（忽略 SIGTERM）; sleep 30"));
    return 0;
}
