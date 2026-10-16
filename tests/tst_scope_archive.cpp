#include "ScopeArchive.h"
#include "ScopeAnalysis.h"
#include "../src/device/ScopeFrame.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <cmath>

class TstScopeArchive : public QObject {
    Q_OBJECT
    static QVariantMap frame() {
        QVariantList samples;
        for (int i=0;i<512;++i) samples.append(int(std::round(2048+1000*std::sin(2*std::acos(-1.0)*i/50))));
        return ScopeFrame::normalized({{"samples",samples},{"rate",50000.0},{"count",512},{"id",42},
            {"signalHz",1000.0},{"vddaMv",3293},{"transferMs",84.5},
            {"source",QStringLiteral("串口测试 / 模拟")},{"receivedAtUtc","2026-10-16T10:20:30.123Z"}});
    }
    static QByteArray read(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
    static void write(const QString &path,const QByteArray &bytes) { QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); QCOMPARE(f.write(bytes),bytes.size()); }
private slots:
    void roundTripAndAnalysis() {
        QTemporaryDir dir; const auto url=QUrl::fromLocalFile(dir.filePath(QStringLiteral("中文 空格.vcscope.json")));
        ScopeArchive archive; const auto original=frame();
        QVERIFY(archive.save(url,original)); QVERIFY(!archive.replaying());
        QVERIFY(archive.load(url)); QVERIFY(archive.replaying());
        const auto replay=archive.replayFrame();
        QCOMPARE(replay.value("samples"),original.value("samples"));
        for (const auto &key:{"rate","count","id","receivedAtUtc","source","vddaMv","transferMs","min","max","mean","measuredHz"})
            QCOMPARE(replay.value(key),original.value(key));
        ScopeAnalysis analysis;
        QCOMPARE(analysis.spectrum(replay),analysis.spectrum(original));
        qInfo()<<"PASS raw ADC, actual rate, UTC receive time, source and derived time/frequency statistics survive replay";
        archive.closeReplay(); QVERIFY(!archive.replaying()); QVERIFY(archive.replayFrame().isEmpty());
    }
    void rejectMalformedWithoutReplacingReplay_data() {
        QTest::addColumn<QString>("kind");
        for (const auto &kind:{"version","format","count","short","range","fraction","string","boolean","rate","id","vdda","timestamp","source","null","truncated","oversize"})
            QTest::newRow(kind)<<QString::fromLatin1(kind);
    }
    void rejectMalformedWithoutReplacingReplay() {
        QFETCH(QString,kind); QTemporaryDir dir;
        ScopeArchive archive; const auto good=QUrl::fromLocalFile(dir.filePath("good.json"));
        QVERIFY(archive.save(good,frame())); QVERIFY(archive.load(good)); const auto before=archive.replayFrame();
        auto root=QJsonDocument::fromJson(read(good.toLocalFile())).object();
        auto a=root.value("acquisition").toObject(); auto samples=root.value("samples").toArray();
        if (kind=="version") root["version"]=2;
        if (kind=="format") root["format"]="unknown";
        if (kind=="count") a["count"]=511.5;
        if (kind=="short") samples.removeLast();
        if (kind=="range") samples[0]=4096;
        if (kind=="fraction") samples[0]=2.5;
        if (kind=="string") samples[0]="2048";
        if (kind=="boolean") samples[0]=true;
        if (kind=="null") samples[0]=QJsonValue::Null;
        if (kind=="rate") a["rate"]=9999;
        if (kind=="id") a["id"]=-1;
        if (kind=="vdda") a["vddaMv"]=-1;
        if (kind=="timestamp") root["receivedAtUtc"]="unknown";
        if (kind=="source") root["source"]=QString(257,'x');
        root["acquisition"]=a; root["samples"]=samples;
        QByteArray bytes=QJsonDocument(root).toJson();
        if (kind=="truncated") bytes.chop(10);
        if (kind=="oversize") bytes=QByteArray(128*1024+1,' ');
        const auto bad=QUrl::fromLocalFile(dir.filePath("bad.json")); write(bad.toLocalFile(),bytes);
        QVERIFY(!archive.load(bad)); QCOMPARE(archive.replayFrame(),before); QVERIFY(!archive.status().isEmpty());
    }
    void rejectInvalidSaveAndIoFailures() {
        QTemporaryDir dir; ScopeArchive archive; const auto url=QUrl::fromLocalFile(dir.filePath("keep.json"));
        QVERIFY(archive.save(url,frame())); const auto before=read(url.toLocalFile());
        auto bad=frame(); bad["vddaMv"]=-1; QVERIFY(!archive.save(url,bad));
        QCOMPARE(read(url.toLocalFile()),before);
        bad=frame(); auto samples=bad.value("samples").toList(); samples[0]=QStringLiteral("2048"); bad["samples"]=samples;
        QVERIFY(!archive.save(url,bad)); QCOMPARE(read(url.toLocalFile()),before);
        QVERIFY(!archive.save(QUrl::fromLocalFile(dir.path()),frame()));
        QVERIFY(!archive.load(QUrl::fromLocalFile(dir.filePath("missing.json"))));
        QVERIFY(!archive.load(QUrl("https://example.com/capture.json")));
        QVERIFY(!archive.save(QUrl("https://example.com/capture.json"),frame()));
    }
    void ignoresForgedDerivedStatistics() {
        QTemporaryDir dir; ScopeArchive archive; const auto url=QUrl::fromLocalFile(dir.filePath("frame.json"));
        QVERIFY(archive.save(url,frame())); auto root=QJsonDocument::fromJson(read(url.toLocalFile())).object();
        auto a=root.value("acquisition").toObject(); a["mean"]=-999; a["measuredHz"]=123456; a["min"]=-8;
        root["acquisition"]=a; write(url.toLocalFile(),QJsonDocument(root).toJson());
        QVERIFY(archive.load(url));
        QCOMPARE(archive.replayFrame().value("mean"),frame().value("mean"));
        QCOMPARE(archive.replayFrame().value("measuredHz"),frame().value("measuredHz"));
    }
    void recordedPhysicalFrameCanBeAnalyzedOffline() {
        ScopeArchive archive;
        QVERIFY(archive.load(QUrl::fromLocalFile(QStringLiteral(VC_SOURCE_DIR "/handbook/system/data/pa4-usb.vcscope.json"))));
        const auto frame=archive.replayFrame();
        QCOMPARE(frame.value("count").toInt(),2048);
        QCOMPARE(frame.value("rate").toDouble(),50000.0);
        QVERIFY(frame.value("source").toString().contains(QStringLiteral("串口")));
        ScopeAnalysis analysis; const auto spectrum=analysis.spectrum(frame);
        QVERIFY(spectrum.value("valid").toBool());
        QVERIFY(std::abs(spectrum.value("peakHz").toDouble()-1000)<spectrum.value("binHz").toDouble());
        qInfo()<<"PASS archived real PA4 frame analyzed offline without device connection";
    }
};
QTEST_GUILESS_MAIN(TstScopeArchive)
#include "tst_scope_archive.moc"
