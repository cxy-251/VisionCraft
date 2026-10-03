// QTimer：定时、防抖、节流
//
// 运行：./example_qt_timers     输出见 output.txt

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include <cstdio>
#include <vector>

static void runFor(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

// 模拟一串连续的事件：每 50 ms 一次，持续 1 秒，之后安静
static void burst(const std::function<void()> &onEvent)
{
    QTimer source;
    int n = 0;
    QObject::connect(&source, &QTimer::timeout, [&] {
        if (++n > 20) { source.stop(); return; }
        onEvent();
    });
    source.start(50);
    runFor(1600);
}

static void debounceVsThrottle()
{
    std::printf("==== 1. 每 50 ms 来一个事件，持续 1 秒：防抖和节流各通知几次 ====\n");
    QElapsedTimer clock;
    clock.start();

    // [region debounce]
    // 防抖：每来一个事件都把计时器重新从头计；安静 200 ms 之后才通知一次
    QTimer debounce;
    debounce.setSingleShot(true);
    debounce.setInterval(200);
    std::vector<qint64> debounceAt;
    QObject::connect(&debounce, &QTimer::timeout, [&] { debounceAt.push_back(clock.elapsed()); });
    // [endregion]

    // [region throttle]
    // 节流：计时器没在跑才启动；第一个事件之后 200 ms 通知一次，期间的事件合并进去
    QTimer throttle;
    throttle.setSingleShot(true);
    throttle.setInterval(200);
    std::vector<qint64> throttleAt;
    QObject::connect(&throttle, &QTimer::timeout, [&] { throttleAt.push_back(clock.elapsed()); });
    // [endregion]

    burst([&] {
        // [region feed]
        debounce.start();                    // 防抖：总是重启
        if (!throttle.isActive())
            throttle.start();                // 节流：没在跑才启动
        // [endregion]
    });

    auto list = [](const std::vector<qint64> &v) {
        QString s;
        for (qint64 t : v) s += QString::number(t / 10 * 10) + QStringLiteral(" ");
        return s;
    };
    std::printf("  防抖：%zu 次，时刻（ms，取整到 10）：%s\n", debounceAt.size(), qPrintable(list(debounceAt)));
    std::printf("  节流：%zu 次，时刻（ms，取整到 10）：%s\n", throttleAt.size(), qPrintable(list(throttleAt)));
}

static void accuracy()
{
    std::printf("\n==== 2. 定时器准不准：100 次 10 ms 的周期，实际间隔 ====\n");
    // [region accuracy]
    for (Qt::TimerType type : {Qt::PreciseTimer, Qt::CoarseTimer}) {
        QTimer t;
        t.setTimerType(type);
        QElapsedTimer clock;
        std::vector<double> gaps;
        qint64 last = -1;
        QEventLoop loop;
        QObject::connect(&t, &QTimer::timeout, [&] {
            const qint64 now = clock.nsecsElapsed();
            if (last >= 0) gaps.push_back((now - last) / 1e6);
            last = now;
            if (gaps.size() == 100) loop.quit();
        });
        clock.start();
        t.start(10);
        loop.exec();
        const auto [mn, mx] = std::minmax_element(gaps.begin(), gaps.end());
        double sum = 0;
        for (double g : gaps) sum += g;
        std::printf("  %-13s 平均 %.2f ms，最短 %.2f，最长 %.2f\n",
                    type == Qt::PreciseTimer ? "PreciseTimer" : "CoarseTimer", sum / gaps.size(), *mn, *mx);
    }
    // [endregion]
}

static void blocked()
{
    std::printf("\n==== 3. 事件循环被占住时，定时器只能等 ====\n");
    // [region blocked]
    QElapsedTimer clock;
    clock.start();
    QTimer t;
    t.setSingleShot(true);
    qint64 firedAt = -1;
    QObject::connect(&t, &QTimer::timeout, [&] { firedAt = clock.elapsed(); });
    t.start(100);                            // 100 ms 后到期
    while (clock.elapsed() < 500) { }        // 主线程忙了 500 ms，没有回到事件循环
    runFor(50);
    std::printf("  设定 100 ms，实际在 %lld ms 时才执行\n", firedAt);
    // [endregion]
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    debounceVsThrottle();
    accuracy();
    blocked();
    return 0;
}
