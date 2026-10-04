// 属性动画：记录动画过程中属性每一次被设置的时间和值
//
// 运行：./example_qt_animation     输出见 output.txt

#include <QCoreApplication>
#include <QEasingCurve>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMetaEnum>
#include <QParallelAnimationGroup>
#include <QPauseAnimation>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QThread>
#include <QTimer>
#include <cstdio>
#include <vector>

// [region target]
// 被动画的对象：只要有 Q_PROPERTY（带 WRITE），就能被 QPropertyAnimation 驱动
class Needle : public QObject {
    Q_OBJECT
    Q_PROPERTY(double value READ value WRITE setValue)
public:
    double value() const { return m_value; }
    void setValue(double v)
    {
        m_value = v;
        log.push_back({clock.nsecsElapsed() / 1e6, v});        // 记下每一次被设置
    }
    struct Sample { double ms, v; };
    std::vector<Sample> log;
    QElapsedTimer clock;
private:
    double m_value = 0;
};
// [endregion]

static void runUntilFinished(QAbstractAnimation *a)
{
    QEventLoop loop;
    QObject::connect(a, &QAbstractAnimation::finished, &loop, &QEventLoop::quit);
    a->start();
    loop.exec();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    std::printf("==== 1. 一个 500 ms 的动画，属性被设置了多少次 ====\n");
    {
        Needle n;
        // [region basic]
        QPropertyAnimation anim(&n, "value");                  // 对象 + 属性名
        anim.setDuration(500);
        anim.setStartValue(0.0);
        anim.setEndValue(100.0);
        // [endregion]
        n.clock.start();
        runUntilFinished(&anim);
        double maxGap = 0;
        for (size_t i = 1; i < n.log.size(); ++i)
            maxGap = std::max(maxGap, n.log[i].ms - n.log[i - 1].ms);
        std::printf("  设置了 %zu 次，平均间隔 %.1f ms，最大间隔 %.1f ms；第一次 %.1f ms 时值 %.2f，最后一次 %.1f ms 时值 %.2f\n",
                    n.log.size(), (n.log.back().ms - n.log.front().ms) / (n.log.size() - 1), maxGap,
                    n.log.front().ms, n.log.front().v, n.log.back().ms, n.log.back().v);
    }

    std::printf("\n==== 2. 缓动曲线：进度 25%%、50%%、75%% 时的值（0→100）====\n");
    // [region easing]
    for (auto type : {QEasingCurve::Linear, QEasingCurve::InOutQuad, QEasingCurve::OutCubic, QEasingCurve::OutBack, QEasingCurve::OutBounce}) {
        QEasingCurve c(type);
        std::printf("  %-10s", QMetaEnum::fromType<QEasingCurve::Type>().valueToKey(type));
        for (double t : {0.25, 0.5, 0.75})
            std::printf("  %6.1f", 100 * c.valueForProgress(t));
        double peak = 0;
        for (int i = 0; i <= 1000; ++i) peak = std::max(peak, 100 * c.valueForProgress(i / 1000.0));
        std::printf("   最大值 %.1f\n", peak);
    }
    // [endregion]

    std::printf("\n==== 3. 事件循环被卡住时 ====\n");
    {
        Needle n;
        QPropertyAnimation anim(&n, "value");
        anim.setDuration(500);
        anim.setStartValue(0.0);
        anim.setEndValue(100.0);
        // [region blocked]
        QTimer::singleShot(100, [] { QThread::msleep(250); });  // 动画进行到 100 ms 时，主线程忙 250 ms
        // [endregion]
        n.clock.start();
        runUntilFinished(&anim);
        double maxGap = 0, before = 0, after = 0;
        for (size_t i = 1; i < n.log.size(); ++i)
            if (n.log[i].ms - n.log[i - 1].ms > maxGap) {
                maxGap = n.log[i].ms - n.log[i - 1].ms;
                before = n.log[i - 1].v;
                after = n.log[i].v;
            }
        std::printf("  设置了 %zu 次；最大间隔 %.1f ms，这期间值从 %.1f 直接跳到 %.1f；总用时 %.0f ms\n",
                    n.log.size(), maxGap, before, after, n.log.back().ms);
    }

    std::printf("\n==== 4. 动画组 ====\n");
    {
        Needle a, b;
        // [region group]
        auto *seq = new QSequentialAnimationGroup;            // 依次执行
        auto *up = new QPropertyAnimation(&a, "value");
        up->setDuration(200); up->setEndValue(100.0);
        auto *down = new QPropertyAnimation(&a, "value");
        down->setDuration(200); down->setEndValue(0.0);         // 不设起点：从动画开始那一刻的当前值出发
        seq->addAnimation(up);
        seq->addPause(100);
        seq->addAnimation(down);

        auto *par = new QParallelAnimationGroup;              // 同时执行
        par->addAnimation(seq);
        auto *other = new QPropertyAnimation(&b, "value");
        other->setDuration(300); other->setEndValue(50.0);
        par->addAnimation(other);                             // 组接管子动画，删组会一起删
        // [endregion]
        a.clock.start();
        b.clock.start();
        runUntilFinished(par);
        auto when = [](const Needle &n, double target, double after = -1) {   // after 之后第一次等于 target 的时间
            for (const auto &s : n.log) if (s.ms > after && qFuzzyCompare(s.v + 1, target + 1)) return s.ms;
            return -1.0;
        };
        const double top = when(a, 100);
        double resume = -1;                                   // 暂停之后第一次被设置
        for (size_t i = 1; i < a.log.size(); ++i)
            if (a.log[i].ms > top && a.log[i].v < 100) { resume = a.log[i].ms; break; }
        std::printf("  a 到达 100 于 %.0f ms，暂停后 %.0f ms 开始下降，回到 0 于 %.0f ms；b 到达 50 于 %.0f ms；组总时长 %d ms\n",
                    top, resume, when(a, 0, top), when(b, 50), par->duration());
        delete par;
    }

    std::printf("\n==== 5. 属性名写错 ====\n");
    {
        Needle n;
        // [region typo]
        QPropertyAnimation anim(&n, "valeu");
        // [endregion]
        anim.setDuration(100);
        anim.setEndValue(100.0);
        n.clock.start();
        runUntilFinished(&anim);
        std::printf("  动画照样跑完（finished 信号照常发出）；value 被设置 %zu 次，仍是 %.0f；动态属性 valeu %s\n",
                    n.log.size(), n.value(), n.dynamicPropertyNames().contains("valeu") ? "被创建了" : "也没有被创建");
    }
    return 0;
}

#include "main.moc"
