#include "ConcurrencyDemo.h"

#include <QtTest>
#include <cstdio>

class TstConcurrencyDemo : public QObject {
    Q_OBJECT
private slots:
    void poolIsFasterAndAgrees()
    {
        ConcurrencyDemo d;
        d.runBlocking();
        const QString blocking = d.lastRun();
        d.runPool();
        QVERIFY(d.running());              // runPool 立即返回，任务还在跑
        QTRY_VERIFY_WITH_TIMEOUT(!d.running() && d.poolMs() > 0, 20000);
        std::printf("\n%s\n%s\n线程池线程数 %d，加速比 %.2f\n", qPrintable(blocking), qPrintable(d.lastRun()),
                    d.threads(), d.blockingMs() / d.poolMs());
        // 两种方式判出的不合格数必须一样（同一批图、同一个算法）
        QCOMPARE(blocking.section("判不合格", 1), d.lastRun().section("判不合格", 1));
        QVERIFY(d.poolMs() < d.blockingMs());
    }
};

QTEST_GUILESS_MAIN(TstConcurrencyDemo)
#include "tst_concurrency_demo.moc"
