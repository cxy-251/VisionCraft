#include "DeviceLink.h"
#include "SimTransport.h"

#include <QtTest>

class TstDeviceLink : public QObject {
    Q_OBJECT

    static SimTransport *sim(DeviceLink &link) { return qobject_cast<SimTransport *>(link.transport()); }

private slots:
    void connectsAndReadsInfo()
    {
        DeviceLink link;
        link.connectSimulator();
        QCOMPARE(link.state(), DeviceLink::Connecting);
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        // 连上后自动 GET_INFO
        QTRY_VERIFY(!link.info().isEmpty());
        QCOMPARE(link.info().value("build").toString(), QString("simulator"));
        QCOMPARE(link.info().value("uid").toString().size(), 24);
    }

    void commandsSucceed()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);

        QSignalSpy finished(&link, &DeviceLink::commandFinished);
        QVERIFY(link.ping() > 0);
        QVERIFY(link.beep(50) > 0);
        QVERIFY(link.setTimeNow() > 0);
        QTRY_COMPARE(finished.size(), 3);
        for (const auto &args : finished)
            QVERIFY2(args.at(1).toBool(), qPrintable(args.at(2).toString()));
    }

    void unknownCommandReportsStatus()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        int status = 999;
        link.request(0x3E, {}, [&](bool ok, int s, const QByteArray &) { QVERIFY(!ok); status = s; });
        QTRY_COMPARE(status, int(VC_ERR_UNKNOWN));
    }

    void telemetryArrives()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy tel(&link, &DeviceLink::telemetryChanged);
        link.subscribeTelemetry(100);
        QTRY_VERIFY_WITH_TIMEOUT(tel.size() >= 3, 2000);
        const double temp = link.telemetry().value("cpuTemp").toDouble();
        QVERIFY(temp > 30 && temp < 40);
    }

    void keyEventsArrive()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy events(&link, &DeviceLink::eventReceived);
        sim(link)->pressKey(VC_KEY1);
        // 只看按键事件（连上时板子还会发一个 hello 事件）
        auto keys = [&] {
            QList<QVariantMap> out;
            for (const auto &args : events) {
                const QVariantMap e = args.at(0).toMap();
                if (e.value("name").toString() == QLatin1String("key"))
                    out << e;
            }
            return out;
        };
        QTRY_COMPARE(keys().size(), 2);
        QCOMPARE(keys()[0].value("key").toInt(), int(VC_KEY1));
        QCOMPARE(keys()[0].value("down").toBool(), true);
        QCOMPARE(keys()[1].value("down").toBool(), false);
    }

    void survivesNoiseOnTheLine()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        sim(link)->injectNoise(QByteArray("\xA5\x5A\x01\xFF\x00\x13garbage\xA5", 16));
        bool ok = false;
        link.request(VC_CMD_PING, "after-noise", [&](bool o, int, const QByteArray &echo) {
            ok = o && echo == "after-noise";
        });
        QTRY_VERIFY(ok);
        QVERIFY(link.stats().value("rxErrors").toUInt() >= 1);
    }

    void throughputTestCompletes()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy done(&link, &DeviceLink::throughputFinished);
        link.runThroughputTest(256, 40, 4);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 5000);
        const QVariantMap r = done.at(0).at(0).toMap();
        QCOMPARE(r.value("ok").toInt(), 40);
        QCOMPARE(r.value("failed").toInt(), 0);
    }

    // 持续有数据时，统计也要定期刷新（限频），而不是等数据停了才刷新一次
    void statsRefreshDuringSustainedTraffic()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy done(&link, &DeviceLink::throughputFinished);
        QSignalSpy stats(&link, &DeviceLink::statsChanged);
        QElapsedTimer t;
        t.start();
        link.runThroughputTest(256, 600, 4);
        QTRY_COMPARE_WITH_TIMEOUT(done.size(), 1, 20000);
        const qint64 ms = t.elapsed();
        const qsizetype during = stats.size();
        qInfo("吞吐测试 %lld ms，期间 statsChanged %lld 次", ms, qlonglong(during));
        QVERIFY(ms > 1000);                       // 测试要足够长，才谈得上「期间」
        QVERIFY(during >= ms / 200 / 2);          // 每 200 ms 最多一次；至少要有一半的机会刷新了
        QVERIFY(during <= ms / 200 + 2);          // 也不能超过限频
    }

    void imageTransferCompletes()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy sent(&link, &DeviceLink::imageSent);
        QImage img(480, 360, QImage::Format_RGB32);
        img.fill(Qt::red);
        QVERIFY(link.sendImage(img));
        QVERIFY(!link.sendImage(img));          // 上一张还在发：拒绝，不排队
        QTRY_COMPARE_WITH_TIMEOUT(sent.size(), 1, 5000);
        QCOMPARE(sent.at(0).at(0).toBool(), true);
        QCOMPARE(sent.at(0).at(1).toInt(), 160 * 120 * 2);   // 480×360 按比例缩到 160×120
        QVERIFY(!link.imageBusy());
    }

    void recipeRoundTrip()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QSignalSpy got(&link, &DeviceLink::recipeReceived);
        QSignalSpy saved(&link, &DeviceLink::recipeSaved);
        link.getRecipe();
        QTRY_COMPARE(got.size(), 1);
        QVERIFY(got.at(0).at(0).toMap().isEmpty());          // 模拟 EEPROM 一开始是空的
        link.setRecipe({{"surfaceThreshold", 22}, {"minDefectArea", 9}, {"maxCenterOffset", 5.5},
                        {"minChipDepth", 4.0}, {"scratchElongation", 3.5}});
        QTRY_COMPARE(saved.size(), 1);
        QVERIFY(saved.at(0).at(0).toBool());
        link.getRecipe();
        QTRY_COMPARE(got.size(), 2);
        const QVariantMap r = got.at(1).at(0).toMap();
        QCOMPARE(r.value("surfaceThreshold").toInt(), 22);
        QCOMPARE(r.value("maxCenterOffset").toDouble(), 5.5);   // ×10 存整数，往返不失真
    }

    void disconnectFailsPendingRequests()
    {
        DeviceLink link;
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        int status = 999;
        link.request(VC_CMD_PING, "x", [&](bool, int s, const QByteArray &) { status = s; });
        link.disconnectDevice();
        QCOMPARE(status, -1);
        QCOMPARE(link.state(), DeviceLink::Disconnected);
        QCOMPARE(link.ping(), -1);
    }
};

QTEST_GUILESS_MAIN(TstDeviceLink)
#include "tst_devicelink.moc"
