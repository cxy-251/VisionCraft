#include "DeviceLink.h"
#include "SimTransport.h"

#include <QtTest>
#include <QJsonDocument>
#include <memory>
#ifdef Q_OS_LINUX
#include <QSocketNotifier>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#endif

class MonitorErrorSim : public SimTransport {
public:
    int mode=0; // 0 normal, 1 unsupported, 2 malformed, 3 silent, 4 delayed old boot
    uint8_t heldSeq=0;
    void write(const QByteArray &bytes) override {
        vc_decoder d;vc_decoder_init(&d);vc_frame f{};
        for(char c:bytes) if(vc_decoder_feed(&d,uint8_t(c),&f) && f.type==VC_CMD_GET_MONITOR && mode) {
            if(mode==3) return;
            if(mode==4) { heldSeq=f.seq;return; }
            QByteArray body(1,mode==1 ? char(VC_ERR_UNKNOWN) : char(VC_OK));
            if(mode==2) body.append("bad schema");
            QByteArray frame(VC_MAX_FRAME,0);
            frame.resize(qsizetype(vc_encode(reinterpret_cast<uint8_t *>(frame.data()),size_t(frame.size()),VC_RSP(f.type),f.seq,
                reinterpret_cast<const uint8_t *>(body.constData()),uint16_t(body.size()))));
            QTimer::singleShot(0,this,[this,frame]{ emit bytesReceived(frame); });return;
        }
        SimTransport::write(bytes);
    }
    void releaseHeld() {
        vc_monitor old{};old.task_status=1;old.task_total=6;
        old.values[VC_MON_cycles_per_ms]=168000;old.values[VC_MON_uptime_ms]=9999999;
        QByteArray body(VC_MONITOR_MAX_SIZE+1,0);
        body.resize(qsizetype(1+vc_monitor_write(reinterpret_cast<uint8_t *>(body.data())+1,size_t(body.size()-1),&old)));
        QByteArray frame(VC_MAX_FRAME,0);
        frame.resize(qsizetype(vc_encode(reinterpret_cast<uint8_t *>(frame.data()),size_t(frame.size()),VC_RSP(VC_CMD_GET_MONITOR),heldSeq,
            reinterpret_cast<const uint8_t *>(body.constData()),uint16_t(body.size()))));
        emit bytesReceived(frame);
    }
    void hello() {
        QByteArray frame(VC_MAX_FRAME,0);
        frame.resize(qsizetype(vc_encode(reinterpret_cast<uint8_t *>(frame.data()),size_t(frame.size()),VC_EVT_HELLO,0,nullptr,0)));
        emit bytesReceived(frame);
    }
};

class ClockErrorSim : public SimTransport {
public:
    explicit ClockErrorSim(uint8_t status) : m_status(status) {}
    void write(const QByteArray &bytes) override
    {
        vc_decoder d; vc_decoder_init(&d);
        vc_frame f{};
        for (const char b : bytes) {
            if (vc_decoder_feed(&d, uint8_t(b), &f) && f.type == VC_CMD_GET_TIME) {
                QByteArray response(VC_MAX_FRAME, 0);
                const size_t n = vc_encode(reinterpret_cast<uint8_t *>(response.data()), response.size(),
                    VC_RSP(f.type), f.seq, &m_status, 1);
                response.resize(qsizetype(n));
                QTimer::singleShot(0, this, [this, response] { emit bytesReceived(response); });
                return;
            }
        }
        SimTransport::write(bytes);
    }
private:
    uint8_t m_status;
};

class BrokenScopeSim : public SimTransport {
public:
    explicit BrokenScopeSim(bool badValue) : m_badValue(badValue) {}
    void write(const QByteArray &bytes) override
    {
        vc_decoder decoder; vc_decoder_init(&decoder); vc_frame f{};
        for (char byte : bytes) {
            if (!vc_decoder_feed(&decoder, uint8_t(byte), &f)) continue;
            if (f.type == VC_CMD_SCOPE_START && f.len == 6) m_total = vc_get_u16(f.payload+4);
            if (f.type != VC_CMD_SCOPE_READ || f.len != 8) continue;
            const auto count = vc_get_u16(f.payload+6);
            QByteArray payload(1+VC_SCOPE_BLOCK_HEADER+2*count, 0);
            auto *p = reinterpret_cast<uint8_t *>(payload.data())+1;
            vc_put_u32(p, vc_get_u32(f.payload));
            vc_put_u16(p+4, vc_get_u16(f.payload+4) + (m_badValue ? 0 : 1));
            vc_put_u16(p+6, m_total);
            if (m_badValue) vc_put_u16(p+8, 5000);
            QByteArray response(VC_MAX_FRAME, 0);
            response.resize(qsizetype(vc_encode(reinterpret_cast<uint8_t *>(response.data()), response.size(),
                VC_RSP(f.type), f.seq, reinterpret_cast<const uint8_t *>(payload.constData()), payload.size())));
            QTimer::singleShot(0, this, [this, response] { emit bytesReceived(response); });
            return;
        }
        SimTransport::write(bytes);
    }
private:
    uint16_t m_total = 0;
    bool m_badValue;
};

class TstDeviceLink : public QObject {
    Q_OBJECT

    static SimTransport *sim(DeviceLink &link) { return qobject_cast<SimTransport *>(link.transport()); }

    // 连上模拟器，并等连接时自动发出的「订阅遥测」也完成。它也会发 commandFinished：
    // 不等它，机器慢时（CI 上实测）它会排在用例自己的命令前面，被误当成用例命令的结果
    static void connectAndSettle(DeviceLink &link)
    {
        QSignalSpy boot(&link, &DeviceLink::commandFinished);
        link.connectSimulator();
        QTRY_COMPARE(link.state(), DeviceLink::Connected);
        QTRY_COMPARE(boot.size(), 1);
        QCOMPARE(boot.at(0).at(0).toString(), QStringLiteral("SUB_TEL"));
    }

private slots:
    void monitorFailureRecoveryAndHistory()
    {
        DeviceLink link;auto *channel=new MonitorErrorSim;
        link.connectWith(channel);
        QTRY_VERIFY(!link.monitor().isEmpty());
        QCOMPARE(link.monitor().value("tasks").toList().size(),6);
        QCOMPARE(link.monitor().value("source").toString(),QStringLiteral("模拟器"));
        for(int i=0;i<125;++i) {
            QSignalSpy changed(&link,&DeviceLink::monitorChanged);
            QVERIFY(link.getMonitor()>=0);QVERIFY(link.getMonitor()<0);
            QTRY_VERIFY(changed.size()>0);
        }
        QCOMPARE(link.monitorHistory().size(),120);
        channel->mode=2;QVERIFY(link.getMonitor()>=0);
        QTRY_VERIFY(link.monitor().isEmpty());QVERIFY(link.monitorSupported());
        channel->mode=0;QVERIFY(link.getMonitor()>=0);QTRY_VERIFY(!link.monitor().isEmpty());
        channel->mode=3;QVERIFY(link.getMonitor()>=0);QTRY_VERIFY_WITH_TIMEOUT(link.monitor().isEmpty(),2000);
        channel->mode=1;QVERIFY(link.getMonitor()>=0);QTRY_VERIFY(!link.monitorSupported());
        QVERIFY(link.getMonitor()<0);
        channel->mode=0;channel->hello();QTRY_VERIFY(!link.monitor().isEmpty());
        QVERIFY(link.monitorHistory().size()<5); // 重启不会接上旧历史。
        channel->mode=4;QVERIFY(link.getMonitor()>=0);
        channel->mode=0;channel->hello();QTRY_VERIFY(!link.monitor().isEmpty());
        const auto fresh=link.monitor();const auto freshHistory=link.monitorHistory();
        channel->releaseHeld();QTest::qWait(20);
        QCOMPARE(link.monitor(),fresh);QCOMPARE(link.monitorHistory(),freshHistory);
        link.disconnectDevice();QVERIFY(link.monitor().isEmpty());QVERIFY(link.monitorHistory().isEmpty());
    }

    void scopeRoundTrip()
    {
        DeviceLink link;
        connectAndSettle(link);
        QSignalSpy finished(&link, &DeviceLink::scopeFinished);
        QVERIFY(link.startScope(50000, 2048));
        QVERIFY(link.scopeBusy());
        QVERIFY(!link.startScope(50000, 512));
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 3000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QVERIFY(!link.scopeBusy());
        const auto f = link.scopeFrame();
        QCOMPARE(f.value("samples").toList().size(), 2048);
        QCOMPARE(f.value("rate").toDouble(), 50000.0);
        QVERIFY(qAbs(f.value("measuredHz").toDouble() - 1000.38) < 5);
        QVERIFY(f.value("min").toInt() < 600);
        QVERIFY(f.value("max").toInt() > 3500);
        link.stopScope();
        QCOMPARE(link.scopeFrame(), f); // 暂停保留完整帧
    }

    void scopeCancelRestart()
    {
        DeviceLink link;
        connectAndSettle(link);
        QVERIFY(link.startScope(10000, 2048));
        QTest::qWait(15);
        link.stopScope();
        QVERIFY(!link.scopeBusy());
        QSignalSpy finished(&link, &DeviceLink::scopeFinished);
        QVERIFY(link.startScope(100000, 512));
        QTRY_COMPARE_WITH_TIMEOUT(finished.size(), 1, 3000);
        QVERIFY2(finished[0][0].toBool(), qPrintable(finished[0][1].toString()));
        QCOMPARE(link.scopeFrame().value("count").toInt(), 512);
        QTest::qWait(250);
        QCOMPARE(finished.size(), 1); // 旧请求/轮询回调不会发布旧帧
        QVERIFY(link.startScope(10000, 2048));
        link.disconnectDevice();
        QVERIFY(!link.scopeBusy());
        QTest::qWait(250);
        QCOMPARE(link.scopeFrame().value("count").toInt(), 512);
    }

    void scopeBadConfiguration()
    {
        DeviceLink link;
        connectAndSettle(link);
        QVERIFY(!link.startScope(0, 2048));
        QVERIFY(!link.startScope(50000, 65536));
        QVERIFY(!link.startScope(50000, 31));
        QVERIFY(!link.scopeBusy());
        QVERIFY(link.scopeFrame().isEmpty());
    }

    void scopeRejectsBrokenBlocks()
    {
        for (bool badValue : {false, true}) {
            DeviceLink link;
            link.connectWith(new BrokenScopeSim(badValue));
            QTRY_COMPARE(link.state(), DeviceLink::Connected);
            QSignalSpy finished(&link, &DeviceLink::scopeFinished);
            QVERIFY(link.startScope(50000, 512));
            QTRY_COMPARE(finished.size(), 1);
            QVERIFY(!finished[0][0].toBool());
            QVERIFY(finished[0][1].toString().contains(badValue ? QStringLiteral("ADC") : QStringLiteral("分块")));
            QVERIFY(!link.scopeBusy());
            QVERIFY(link.scopeFrame().isEmpty()); // 不发布部分帧或越界值
        }
    }

    void scopeBoardRestart()
    {
        DeviceLink link;
        connectAndSettle(link);
        QSignalSpy finished(&link, &DeviceLink::scopeFinished);
        QVERIFY(link.startScope(10000, 2048));
        QTest::qWait(10);
        QByteArray hello(VC_HEADER_SIZE + VC_TRAILER_SIZE, 0);
        vc_encode(reinterpret_cast<uint8_t *>(hello.data()), hello.size(), VC_EVT_HELLO, 0, nullptr, 0);
        sim(link)->injectNoise(hello); // 有效 HELLO 模拟采集中收到板子重启通知
        QTRY_COMPARE(finished.size(), 1);
        QVERIFY(!finished[0][0].toBool());
        QVERIFY(!link.scopeBusy());
        QVERIFY(link.scopeFrame().isEmpty());
        QVERIFY(link.scopeStatus().contains(QStringLiteral("重启")));
    }
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
        connectAndSettle(link);

        QSignalSpy finished(&link, &DeviceLink::commandFinished);
        QVERIFY(link.ping() > 0);
        QVERIFY(link.beep(50) > 0);
        QTRY_COMPARE(finished.size(), 2);
        for (const auto &args : finished)
            QVERIFY2(args.at(1).toBool(), qPrintable(args.at(2).toString()));
    }

    // [region sim-matches-firmware]
    // 正式固件已接入 RTC，模拟器同样从未校时开始，对时后能读回并走时。
    void setTimeAnswersLikeFirmware()
    {
        DeviceLink link;
        connectAndSettle(link);
        QTRY_VERIFY(link.deviceClock().value("available").toBool());
        QCOMPARE(link.deviceClock().value("valid").toBool(), false);
        QSignalSpy finished(&link, &DeviceLink::commandFinished);
        QVERIFY(link.setTimeNow() > 0);
        QTRY_COMPARE(finished.size(), 1);
        QCOMPARE(finished.at(0).at(1).toBool(), true);
        QTRY_VERIFY(link.deviceClock().value("valid").toBool());
        const QString before = link.deviceClock().value("text").toString();
        QTest::qWait(1100);
        QVERIFY(link.getTime() > 0);
        QTRY_VERIFY(link.deviceClock().value("text").toString() != before);
        link.disconnectDevice();
        QVERIFY(link.deviceClock().isEmpty());
    }
    // [endregion]

    void clockReadErrors()
    {
        for (uint8_t status : {VC_ERR_UNKNOWN, VC_ERR_IO, VC_OK}) {
            DeviceLink link;
            link.connectWith(new ClockErrorSim(status));
            QTRY_VERIFY(link.deviceClock().contains("available"));
            QVERIFY(!link.deviceClock().value("available").toBool());
            QVERIFY(!link.deviceClock().value("error").toString().isEmpty());
        } // VC_OK + 无数据是畸形应答，同样不能展示有效时间
    }

    void unknownCommandReportsStatus()
    {
        DeviceLink link;
        connectAndSettle(link);
        int status = 999;
        link.request(0x3E, {}, [&](bool ok, int s, const QByteArray &) { QVERIFY(!ok); status = s; });
        QTRY_COMPARE(status, int(VC_ERR_UNKNOWN));
    }

    void telemetryArrives()
    {
        DeviceLink link;
        connectAndSettle(link);
        QSignalSpy tel(&link, &DeviceLink::telemetryChanged);
        link.subscribeTelemetry(100);
        QTRY_VERIFY_WITH_TIMEOUT(tel.size() >= 3, 2000);
        const double temp = link.telemetry().value("cpuTemp").toDouble();
        QVERIFY(temp > 30 && temp < 40);
    }

    void keyEventsArrive()
    {
        DeviceLink link;
        connectAndSettle(link);
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
        connectAndSettle(link);
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
        connectAndSettle(link);
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
        connectAndSettle(link);
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
        connectAndSettle(link);
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
        connectAndSettle(link);
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

    void shellRunsCommands()
    {
        DeviceLink link;
        connectAndSettle(link);
        QSignalSpy spy(&link, &DeviceLink::shellReply);
        link.shell("help");
        QTRY_COMPARE(spy.size(), 1);
        QVERIFY(spy.at(0).at(1).toBool());
        const QStringList lines = spy.at(0).at(2).toString().split('\n');
        QVERIFY(lines.contains("link     transport and uart counters"));
        QVERIFY(lines.first().startsWith("help     list commands"));
        link.shell("frobnicate");
        QTRY_COMPARE(spy.size(), 2);
        QCOMPARE(spy.at(1).at(2).toString(), QString("unknown command: frobnicate (try help)"));
        QCOMPARE(link.shell(QString("x").repeated(64)), -1);     // 超长：不发，直接报错
        QCOMPARE(link.shell(QString::fromUtf8("帮助")), -1);      // 板子只认 ASCII
        QCOMPARE(spy.size(), 4);
        QVERIFY(!spy.at(3).at(1).toBool());
    }

    void serialRealBoard()
    {
        const QString port=qEnvironmentVariable("VC_SERIAL_TEST_PORT");
        if (port.isEmpty()) QSKIP("Opt-in physical USB_232 test; set VC_SERIAL_TEST_PORT");
        DeviceLink link;
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),5000);
        QTRY_VERIFY_WITH_TIMEOUT(!link.monitor().isEmpty(),5000);
        qInfo().noquote()<<"SERIAL INFO"<<QJsonDocument::fromVariant(link.info()).toJson(QJsonDocument::Compact);
        QSignalSpy shell(&link,&DeviceLink::shellReply);
        QString initialStats;
        for (const QString &cmd : {QStringLiteral("link"),QStringLiteral("last")}) {
            shell.clear(); link.shell(cmd);
            QTRY_COMPARE_WITH_TIMEOUT(shell.size(),1,3000);
            QVERIFY(shell.first().at(1).toBool());
            qInfo().noquote()<<cmd<<shell.first().at(2).toString();
            if (cmd=="link") {
                initialStats=shell.first().at(2).toString();
                QVERIFY(initialStats.contains("active UART"));
            }
        }
        QTRY_VERIFY_WITH_TIMEOUT(!link.telemetry().isEmpty(),3000);
        QVERIFY(link.deviceClock().value("available").toBool());
        qInfo().noquote()<<"SERIAL TELEMETRY"<<QJsonDocument::fromVariant(link.telemetry()).toJson(QJsonDocument::Compact);
        qInfo().noquote()<<"SERIAL CLOCK"<<QJsonDocument::fromVariant(link.deviceClock()).toJson(QJsonDocument::Compact);
        const uint32_t initialUptime=link.monitor().value("uptime_ms").toUInt();
        for (int i=0;i<10;++i) {
            const auto result=std::make_shared<std::pair<bool,bool>>(false,false);
            const QByteArray payload(1023,char(i));
            link.request(VC_CMD_PING,payload,[result,payload](bool ok,int,const QByteArray &reply) { result->second=ok && reply==payload; result->first=true; },3000);
            QTRY_VERIFY_WITH_TIMEOUT(result->first,4000); QVERIFY(result->second);
            const uint32_t previous=link.scopeFrame().value("id").toUInt();
            QVERIFY(link.startScope(50000,1024));
            QTRY_VERIFY_WITH_TIMEOUT(link.scopeFrame().value("id").toUInt()>previous,5000);
            QCOMPARE(link.scopeFrame().value("samples").toList().size(),1024);
            link.getMonitor();
            QTest::qWait(150);
        }
        QSignalSpy image(&link,&DeviceLink::imageSent);
        QImage pixels(160,120,QImage::Format_RGB32); pixels.fill(Qt::darkCyan);
        QVERIFY(link.sendImage(pixels));
        QTRY_COMPARE_WITH_TIMEOUT(image.size(),1,10000);
        QVERIFY(image.first().at(0).toBool());
        QCOMPARE(image.first().at(1).toInt(),38400);
        qInfo()<<"SERIAL IMAGE bytes"<<image.first().at(1).toInt()<<"ms"<<image.first().at(2).toDouble();
        for (int i=0;i<3;++i) {
            link.disconnectDevice(); link.connectSerial(port,115200);
            QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),5000);
            QTRY_VERIFY_WITH_TIMEOUT(!link.monitor().isEmpty(),5000);
        }
        QVERIFY(link.monitor().value("uptime_ms").toUInt()>initialUptime);
        QCOMPARE(link.monitor().value("fault_kind").toUInt(),0u);
        QVERIFY(!(link.monitor().value("reset_flags").toUInt() & (1u<<29)));
        qInfo().noquote()<<"SERIAL FINAL MONITOR"<<QJsonDocument::fromVariant(link.monitor()).toJson(QJsonDocument::Compact);
        shell.clear();link.shell("link");
        QTRY_COMPARE_WITH_TIMEOUT(shell.size(),1,3000);
        QVERIFY(shell.first().at(1).toBool());
        const QString stats=shell.first().at(2).toString();
        qInfo().noquote()<<"SERIAL FINAL"<<stats;
        const QRegularExpression counters(QStringLiteral("uart rx-drop ([0-9]+) tx-drop ([0-9]+) errors ([0-9]+)"));
        const auto before=counters.match(initialStats), after=counters.match(stats);
        QVERIFY(before.hasMatch() && after.hasMatch());
        // Preserve cumulative errors from earlier runs/cable changes. Recovery
        // must add no new errors; never reset the board just to make counters zero.
        for (int i=1;i<=3;++i) QCOMPARE(after.captured(i),before.captured(i));
        link.stopScope();link.disconnectDevice();
        qInfo()<<"PASS physical USB_232: ten frames, ten maximum PINGs, 38400-byte image, telemetry, RTC, three client reconnects; no SD operations";
    }

    void serialStandaloneReboot()
    {
        const QString port=qEnvironmentVariable("VC_SERIAL_TEST_PORT");
        if (port.isEmpty() || qEnvironmentVariableIntValue("VC_SERIAL_TEST_REBOOT")!=1)
            QSKIP("Opt-in standalone software reset; set VC_SERIAL_TEST_REBOOT=1 as well as the serial port");
        DeviceLink link;
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),5000);
        const QString uid=link.info().value("uid").toString();
        const auto uptime=link.info().value("uptimeMs").toUInt();
        QSignalSpy shell(&link,&DeviceLink::shellReply);
        link.shell("diag");
        QTRY_COMPARE_WITH_TIMEOUT(shell.size(),1,3000);
        QVERIFY(shell.first().at(1).toBool());
        const QString state=shell.first().at(2).toString();
        QVERIFY(state.contains("active 0") && !state.contains("pending 1"));
        qInfo().noquote()<<"PRE-RESET diagnostic state"<<state;
        shell.clear();link.shell("reboot");
        QTRY_COMPARE_WITH_TIMEOUT(shell.size(),1,3000);
        QVERIFY(shell.first().at(1).toBool());
        QVERIFY(shell.first().at(2).toString().contains("rebooting"));
        link.disconnectDevice();
        QTest::qWait(3000);
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),5000);
        QTRY_VERIFY_WITH_TIMEOUT(!link.monitor().isEmpty(),5000);
        QCOMPARE(link.info().value("uid").toString(),uid);
        QVERIFY(link.info().value("uptimeMs").toUInt()<uptime);
        QVERIFY(!(link.monitor().value("reset_flags").toUInt() & (1u<<29)));
        QCOMPARE(link.monitor().value("fault_kind").toUInt(),0u);
        QVERIFY(link.monitor().value("reset_flags").toUInt() & (1u<<28));
        qInfo().noquote()<<"STANDALONE INFO"<<QJsonDocument::fromVariant(link.info()).toJson(QJsonDocument::Compact);
        qInfo().noquote()<<"STANDALONE MONITOR"<<QJsonDocument::fromVariant(link.monitor()).toJson(QJsonDocument::Compact);
        shell.clear();link.shell("link");
        QTRY_COMPARE_WITH_TIMEOUT(shell.size(),1,3000);
        qInfo().noquote()<<"STANDALONE LINK"<<shell.first().at(2).toString();
        QVERIFY(shell.first().at(1).toBool());
        QVERIFY(shell.first().at(2).toString().contains("uart rx-drop 0 tx-drop 0 errors 0"));
        link.disconnectDevice();
        qInfo()<<"PASS software reset and reconnect over USB_232 without ST-Link; no SD operations; not a power-cycle test";
    }

    void serialPseudoTerminalRoundTripAndRecovery()
    {
#ifndef Q_OS_LINUX
        QSKIP("POSIX pseudo-terminal test is Linux only");
#else
        const int master=posix_openpt(O_RDWR|O_NOCTTY|O_NONBLOCK);
        QVERIFY(master>=0);
        struct Guard { int fd; ~Guard() { ::close(fd); } } guard{master};
        QVERIFY(grantpt(master)==0 && unlockpt(master)==0);
        const QString port=QString::fromLocal8Bit(ptsname(master));
        SimTransport board;
        bool silent=false;
        QByteArray outgoing;
        QSocketNotifier input(master,QSocketNotifier::Read);
        connect(&input,&QSocketNotifier::activated,this,[&] {
            char buf[2048]; ssize_t n;
            while ((n=::read(master,buf,sizeof buf))>0)
                if (!silent) board.write(QByteArray(buf,n));
        });
        connect(&board,&Transport::bytesReceived,this,[&](const QByteArray &bytes) {
            if (!silent) outgoing+=bytes;
        });
        // Fragment board output into 7-byte pieces, including protocol headers.
        QTimer fragment;
        fragment.setInterval(1);
        connect(&fragment,&QTimer::timeout,this,[&] {
            if (outgoing.isEmpty()) return;
            const auto n=::write(master,outgoing.constData(),size_t(qMin(7,outgoing.size())));
            if (n>0) outgoing.remove(0,n);
        });
        fragment.start(); board.open();
        DeviceLink link;
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),3000);
        QTRY_VERIFY_WITH_TIMEOUT(!link.monitor().isEmpty(),3000);
        QSignalSpy replies(&link,&DeviceLink::shellReply);
        link.shell("help");
        QTRY_COMPARE_WITH_TIMEOUT(replies.size(),1,3000);
        QVERIFY(replies.first().at(1).toBool());
        QByteArray payload(1023,'x'); payload[511]=0;
        bool echoed=false;
        link.request(VC_CMD_PING,payload,[&](bool ok,int,const QByteArray &body) { echoed=ok && body==payload; },3000);
        QTRY_VERIFY_WITH_TIMEOUT(echoed,4000);
        QVERIFY(link.startScope(50000,1024));
        QTRY_VERIFY_WITH_TIMEOUT(!link.scopeFrame().isEmpty(),5000);
        link.disconnectDevice();
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        termios settings{};
        QVERIFY(tcgetattr(master,&settings)==0);
        QVERIFY(!(settings.c_lflag & ECHO));
        QCOMPARE(cfgetospeed(&settings),speed_t(B115200));
#endif
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),3000);
        // Adapter still exists, but board no longer answers. Heartbeat must
        // clear Connected even with no operating-system unplug error.
        silent=true; outgoing.clear();
        QTRY_COMPARE_WITH_TIMEOUT(link.state(),DeviceLink::Disconnected,7500);
        QVERIFY(link.monitor().isEmpty());
        silent=false;
        link.connectSerial(port,115200);
        QTRY_VERIFY_WITH_TIMEOUT(!link.info().isEmpty(),3000);
        link.disconnectDevice(); board.close();
#endif
    }

    void disconnectFailsPendingRequests()
    {
        DeviceLink link;
        connectAndSettle(link);
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
