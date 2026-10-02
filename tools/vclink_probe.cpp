// 命令行探针：用和上位机完全相同的 DeviceLink 连接板子，跑一遍基本命令和吞吐测试。
//   vclink_probe [rtt|sim]
// 没有界面、不需要人点按钮，用来在板子上验证整条链路。
#include "DeviceLink.h"

#include <QCoreApplication>
#include <QTimer>
#include <cstdio>

static void say(const QString &s)
{
    std::printf("%s\n", qPrintable(s));
    std::fflush(stdout);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString mode = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral("rtt");
    // 第二个参数 soak：只做一次长时间吞吐测试（2000 个 512 字节 PING）
    const bool soak = argc > 2 && QByteArray(argv[2]) == "soak";

    DeviceLink link;
    QObject::connect(&link, &DeviceLink::logLine, [](const QString &kind, const QString &text) {
        say(QStringLiteral("[%1] %2").arg(kind, -6).arg(text));
    });

    int telemetryCount = 0;
    QObject::connect(&link, &DeviceLink::telemetryChanged, [&] {
        const QVariantMap t = link.telemetry();
        if (t.isEmpty())
            return;
        ++telemetryCount;
        say(QStringLiteral("[tel   ] 温度 %1 °C  光照 %2 %  VDDA %3 mV  板子收帧 %4 / 坏帧 %5")
                .arg(t.value("cpuTemp").toDouble(), 0, 'f', 2)
                .arg(t.value("light").toDouble(), 0, 'f', 1)
                .arg(t.value("vddaMv").toInt())
                .arg(t.value("deviceRxFrames").toUInt())
                .arg(t.value("deviceRxErrors").toUInt()));
    });

    // 依次执行的步骤
    QList<std::function<void()>> steps;
    int stepIndex = 0;
    auto next = [&] {
        if (stepIndex < steps.size())
            steps[stepIndex++]();
        else
            app.exit(0);
    };

    if (soak) {
        steps << [&] {
            QObject::connect(&link, &DeviceLink::throughputFinished, [&](const QVariantMap &r) {
                app.exit(r.value("failed").toInt() == 0 ? 0 : 3);
            });
            link.runThroughputTest(512, 2000, 4);
        };
    }
    steps << [&] { link.ping(); QTimer::singleShot(500, next); };
    steps << [&] { link.beep(80); QTimer::singleShot(500, next); };
    steps << [&] { link.setTimeNow(); QTimer::singleShot(500, next); };   // 预期：RTC 未配置，返回「不认识的命令」
    steps << [&] { link.subscribeTelemetry(250); QTimer::singleShot(1600, next); };
    steps << [&] { link.subscribeTelemetry(0); QTimer::singleShot(300, next); };
    for (int size : {16, 256, 1000}) {
        steps << [&, size] {
            auto *conn = new QMetaObject::Connection;
            *conn = QObject::connect(&link, &DeviceLink::throughputFinished, [&, conn](const QVariantMap &) {
                QObject::disconnect(*conn);
                delete conn;
                QTimer::singleShot(200, next);
            });
            link.runThroughputTest(size, size > 500 ? 40 : 100, 4);
        };
    }
    steps << [&] {
        const QVariantMap s = link.stats();
        say(QStringLiteral("[stats ] 上位机发 %1 帧，收 %2 帧，坏帧 %3；遥测 %4 次")
                .arg(s.value("txFrames").toULongLong()).arg(s.value("rxFrames").toUInt())
                .arg(s.value("rxErrors").toUInt()).arg(telemetryCount));
        next();
    };

    QObject::connect(&link, &DeviceLink::stateChanged, [&] {
        if (link.state() == DeviceLink::Connected && stepIndex == 0)
            QTimer::singleShot(400, next);   // 先等自动 GET_INFO 回来
        else if (link.state() == DeviceLink::Disconnected && !link.statusText().isEmpty() && stepIndex == 0) {
            say(QStringLiteral("[fail  ] ") + link.statusText());
            app.exit(1);
        }
    });

    QTimer::singleShot(60000, &app, [&] { say(QStringLiteral("[fail  ] 超时")); app.exit(2); });

    if (mode == QLatin1String("sim"))
        link.connectSimulator();
    else
        link.connectRtt();
    return app.exec();
}
