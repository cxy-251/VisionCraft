#include "ScopeAnalysis.h"
#include <QElapsedTimer>
#include <QFile>
#include <QtTest>
#include <cmath>
#include <limits>

namespace {
QVariantMap tone(int n, double bin, double amplitude = 1000, double rate = 50000)
{
    QVariantList samples;
    for (int i = 0; i < n; ++i)
        samples.append(std::round(2048 + amplitude * std::cos(2 * std::acos(-1.0) * bin * i / n)));
    return {{"samples", samples}, {"count", n}, {"rate", rate}};
}
}

class TstScopeAnalysis : public QObject {
    Q_OBJECT
private slots:
    void coherentToneAndDcOffset()
    {
        ScopeAnalysis analyzer;
        const auto result = analyzer.spectrum(tone(1024, 20));
        QVERIFY(result.value("valid").toBool());
        QCOMPARE(result.value("hz").toList().size(), 513);
        QCOMPARE(result.value("binHz").toDouble(), 50000.0 / 1024);
        QCOMPARE(result.value("peakHz").toDouble(), 20 * 50000.0 / 1024);
        QVERIFY(std::abs(result.value("peakAmplitude").toDouble() - 1000) < 1);
        QVERIFY(std::abs(result.value("mean").toDouble() - 2048) < 1);
        QVERIFY(std::abs(result.value("acRms").toDouble() - 1000 / std::sqrt(2.0)) < 1);
        qInfo("coherent tone: peak %.3f Hz, amplitude %.3f ADC, AC RMS %.3f ADC",
              result.value("peakHz").toDouble(), result.value("peakAmplitude").toDouble(), result.value("acRms").toDouble());
    }

    void constantAndNyquist()
    {
        ScopeAnalysis analyzer;
        const auto dc = analyzer.spectrum(tone(512, 0, 500));
        QVERIFY(dc.value("valid").toBool());
        QVERIFY(!dc.value("hasPeak").toBool());
        QCOMPARE(dc.value("acRms").toDouble(), 0);
        const auto nyquist = analyzer.spectrum(tone(512, 256, 600));
        QCOMPARE(nyquist.value("peakHz").toDouble(), 25000);
        QVERIFY(std::abs(nyquist.value("peakAmplitude").toDouble() - 600) < 1e-6);
        QCOMPARE(nyquist.value("acRms").toDouble(), 600);
        // 奇数长度末点仍乘二；此余弦的正/负频率相邻，Hann 窗叠加后幅值为 300，不能当 Nyquist 再减半。
        const auto odd = analyzer.spectrum(tone(513, 256, 600));
        QVERIFY(odd.value("valid").toBool());
        QVERIFY(std::abs(odd.value("peakAmplitude").toDouble() - 300) < 1);
    }

    void noncoherentAndTwoTones()
    {
        ScopeAnalysis analyzer;
        const auto off = analyzer.spectrum(tone(2048, 40.4));
        QVERIFY(std::abs(off.value("peakHz").toDouble() - 40.4 * 50000 / 2048) <= off.value("binHz").toDouble());
        QVERIFY(off.value("peakAmplitude").toDouble() < 1000); // Hann 降低泄漏，非整数周期仍有幅值损失。
        auto frame = tone(2048, 40, 1000);
        auto samples = frame.value("samples").toList();
        for (int i = 0; i < samples.size(); ++i)
            samples[i] = samples[i].toDouble() + std::round(300 * std::cos(2 * std::acos(-1.0) * 120 * i / samples.size()));
        frame["samples"] = samples;
        const auto two = analyzer.spectrum(frame);
        const auto amplitudes = two.value("amplitudes").toList();
        QVERIFY(std::abs(amplitudes[40].toDouble() - 1000) < 1);
        QVERIFY(std::abs(amplitudes[120].toDouble() - 300) < 1);
        QCOMPARE(two.value("peakHz").toDouble(), 40 * 50000.0 / 2048);
    }

    void invalidFrameDoesNotKeepOldSpectrum()
    {
        ScopeAnalysis analyzer;
        QVERIFY(analyzer.spectrum(tone(1024, 20)).value("valid").toBool());
        const auto reject = [&analyzer](const QVariantMap &frame) {
            const auto result = analyzer.spectrum(frame);
            return !result.value("valid").toBool() && !result.contains("amplitudes") && !result.value("error").toString().isEmpty();
        };
        QVERIFY(reject({}));
        QVERIFY(reject(tone(31, 2)));
        QVERIFY(reject(tone(2049, 20)));
        for (double rate : {0.0, 9999.0, 200001.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            auto frame = tone(512, 20); frame["rate"] = rate; QVERIFY(reject(frame));
        }
        auto frame = tone(512, 20); frame["count"] = 513; QVERIFY(reject(frame));
        for (QVariant bad : {QVariant(-1), QVariant(4096), QVariant(1.5), QVariant("bad"), QVariant(std::numeric_limits<double>::quiet_NaN())}) {
            frame = tone(512, 20); auto samples = frame["samples"].toList(); samples[12] = bad; frame["samples"] = samples;
            QVERIFY(reject(frame));
        }
    }

    void savedBoardFrame()
    {
        QFile file(QStringLiteral(VC_SOURCE_DIR "/handbook/f407/analog/scope-stream-200k.txt"));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVariantList samples;
        while (!file.atEnd()) {
            bool ok = false; const int value = file.readLine().trimmed().toInt(&ok);
            QVERIFY(ok); samples.append(value);
        }
        QCOMPARE(samples.size(), 2048);
        ScopeAnalysis analyzer;
        const QVariantMap frame{{"samples", samples}, {"count", samples.size()}, {"rate", 200000}};
        const auto result = analyzer.spectrum(frame);
        QVERIFY(result.value("valid").toBool());
        QVERIFY(std::abs(result.value("peakHz").toDouble() - 1000.381) < result.value("binHz").toDouble());
        QVERIFY(result.value("acRms").toDouble() > 1000 && result.value("acRms").toDouble() < 1300);
        QElapsedTimer timer; timer.start();
        for (int i = 0; i < 100; ++i) QVERIFY(analyzer.spectrum(frame).value("valid").toBool());
        qInfo("saved board frame: N=2048 Fs=200000, bin %.5f Hz, peak %.5f Hz, amplitude %.3f ADC, AC RMS %.3f ADC; 100 analyses %.3f ms",
              result.value("binHz").toDouble(), result.value("peakHz").toDouble(), result.value("peakAmplitude").toDouble(),
              result.value("acRms").toDouble(), timer.nsecsElapsed()/1e6);
    }
};
QTEST_GUILESS_MAIN(TstScopeAnalysis)
#include "tst_scope_analysis.moc"
