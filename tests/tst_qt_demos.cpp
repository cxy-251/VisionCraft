#include "DeadlockDemo.h"
#include "SignalTraceDemo.h"

#include <QtTest>

// 把某一轮（seq）的事件按发生顺序取出来，每条写成 "kind@线程"，例如 "slot@main"
static QStringList round(const SignalTraceDemo &demo, int seq)
{
    QStringList out;
    for (const QVariant &v : demo.events()) {
        const QVariantMap e = v.toMap();
        if (e.value("seq").toInt() == seq)
            out << e.value("kind").toString() + "@" + (e.value("main").toBool() ? "main" : "worker");
    }
    return out;
}

class TstQtDemos : public QObject {
    Q_OBJECT

private slots:
    void directFromWorkerRunsSlotInWorker()
    {
        SignalTraceDemo demo;
        demo.setConnectionType(SignalTraceDemo::Direct);
        demo.emitFromWorker();
        QTRY_COMPARE(round(demo, 1).size(), 4);
        // 槽在工作线程里同步执行，emit 等它执行完才返回
        QCOMPARE(round(demo, 1),
                 QStringList({"emit@worker", "slot@worker", "slotEnd@worker", "return@worker"}));
    }

    void queuedFromWorkerRunsSlotInMain()
    {
        SignalTraceDemo demo;
        demo.setConnectionType(SignalTraceDemo::Queued);
        demo.setSlotDelayMs(100);
        demo.emitFromWorker();
        QTRY_COMPARE(round(demo, 1).size(), 4);
        const QStringList r = round(demo, 1);
        QCOMPARE(r.first(), QString("emit@worker"));
        QVERIFY(r.contains("slot@main"));
        // 槽耗时 100 ms，而 emit 不等它：emit 返回一定排在槽执行完之前
        QVERIFY(r.indexOf("return@worker") < r.indexOf("slotEnd@main"));
    }

    void autoFromWorkerBehavesLikeQueued()
    {
        SignalTraceDemo demo;   // 默认 Auto
        demo.setSlotDelayMs(100);
        demo.emitFromWorker();
        QTRY_COMPARE(round(demo, 1).size(), 4);
        const QStringList r = round(demo, 1);
        QVERIFY(r.contains("slot@main"));
        QVERIFY(r.indexOf("return@worker") < r.indexOf("slotEnd@main"));
    }

    void autoFromMainBehavesLikeDirect()
    {
        SignalTraceDemo demo;
        demo.emitFromMain();
        QTRY_COMPARE(round(demo, 1).size(), 4);
        QCOMPARE(round(demo, 1), QStringList({"emit@main", "slot@main", "slotEnd@main", "return@main"}));
    }

    void blockingQueuedFromWorkerWaitsForSlot()
    {
        SignalTraceDemo demo;
        demo.setConnectionType(SignalTraceDemo::BlockingQueued);
        demo.emitFromWorker();
        QTRY_COMPARE(round(demo, 1).size(), 4);
        // 槽在主线程执行，工作线程一直等到它执行完
        QCOMPARE(round(demo, 1), QStringList({"emit@worker", "slot@main", "slotEnd@main", "return@worker"}));
    }

    void blockingQueuedFromMainIsRefused()
    {
        SignalTraceDemo demo;
        demo.setConnectionType(SignalTraceDemo::BlockingQueued);
        demo.emitFromMain();
        QTRY_COMPARE(round(demo, 1).size(), 1);
        QCOMPARE(round(demo, 1), QStringList({"warn@main"}));
    }

    // 必须放最后：会永久卡死一个线程
    void deadlockDemoReallyDeadlocks()
    {
        DeadlockDemo demo;
        QCOMPARE(demo.state(), DeadlockDemo::Idle);
        demo.trigger();
        QCOMPARE(demo.state(), DeadlockDemo::Running);
        QTRY_COMPARE_WITH_TIMEOUT(demo.state(), DeadlockDemo::Deadlocked, 3000);
        QVERIFY2(demo.qtWarning().contains("Dead lock detected"), qPrintable(demo.qtWarning()));

        // 第二次触发被拒绝
        DeadlockDemo again;
        QCOMPARE(again.state(), DeadlockDemo::AlreadyUsed);
    }
};

QTEST_GUILESS_MAIN(TstQtDemos)
#include "tst_qt_demos.moc"
