#include "Inspector.h"
#include "PartGenerator.h"

#include <QtTest>
#include <cstdio>

// 用模拟产线的「标准答案」统计检测算法的混淆矩阵：行 = 真实缺陷，列 = 判定结果
class TstInspector : public QObject {
    Q_OBJECT

private slots:
    void confusionMatrix_data()
    {
        QTest::addColumn<double>("difficulty");
        QTest::addColumn<double>("required");   // 每一类至少要达到的准确率
        QTest::newRow("容易") << 0.0 << 0.97;
        QTest::newRow("中等") << 0.5 << 0.90;
        QTest::newRow("困难") << 1.0 << 0.0;    // 只打印矩阵，不要求：故意接近阈值
    }

    void confusionMatrix()
    {
        QFETCH(double, difficulty);
        QFETCH(double, required);
        const int perClass = qEnvironmentVariableIsSet("VC_INSPECT_N") ? qEnvironmentVariableIntValue("VC_INSPECT_N") : 200;
        PartGenerator gen(12345);
        Inspector inspector;
        int m[VC_DEFECT_COUNT][VC_DEFECT_COUNT] = {};
        double totalMs = 0;
        for (int truth = VC_DEFECT_NONE; truth <= VC_DEFECT_OFFSET; ++truth) {
            for (int i = 0; i < perClass; ++i) {
                const auto part = gen.make(static_cast<vc_defect>(truth), difficulty);
                const auto r = inspector.inspect(part.image);
                m[truth][r.defect]++;
                totalMs += r.ms;
            }
        }
        std::printf("\n难度 %.1f\n真实\\判定   合格  划痕  缺口  污点  偏心  缺失\n", difficulty);
        const char *names[] = {"合格", "划痕", "缺口", "污点", "偏心"};
        int correct = 0;
        for (int t = 0; t <= VC_DEFECT_OFFSET; ++t) {
            std::printf("%-8s", names[t]);
            for (int p = 0; p < VC_DEFECT_COUNT; ++p)
                std::printf("%6d", m[t][p]);
            std::printf("   准确率 %.1f%%\n", 100.0 * m[t][t] / perClass);
            correct += m[t][t];
        }
        std::printf("总体准确率 %.2f%%，平均每张 %.2f ms\n", 100.0 * correct / (5 * perClass), totalMs / (5 * perClass));
        for (int t = 0; t <= VC_DEFECT_OFFSET; ++t)
            QVERIFY2(m[t][t] >= perClass * required, names[t]);
    }

    // 阈值扫描：同一批「接近阈值」的工件，表面阈值从低到高，看漏检和误报怎么变化（只打印，不判定）
    void thresholdSweep()
    {
        const int perClass = 120;
        std::printf("\n难度 0.8，表面阈值扫描（每类 %d 件）\n阈值  判对率  漏检率  误报率\n", perClass);
        for (int threshold : {14, 16, 17, 18, 19, 20, 22, 25, 30, 40, 60}) {
            PartGenerator gen(2026);
            Inspector inspector;
            inspector.params.surfaceThreshold = threshold;
            // [region sweep]
            int correct = 0, escape = 0, falseReject = 0;
            for (int t = VC_DEFECT_NONE; t <= VC_DEFECT_OFFSET; ++t) {
                for (int i = 0; i < perClass; ++i) {
                    const auto r = inspector.inspect(gen.make(static_cast<vc_defect>(t), 0.8).image);
                    correct += r.defect == t;
                    if (t != VC_DEFECT_NONE && r.ok) escape++;
                    if (t == VC_DEFECT_NONE && !r.ok) falseReject++;
                }
            }
            // [endregion]
            std::printf("%4d  %5.1f%%  %5.1f%%  %5.1f%%\n", threshold, 100.0 * correct / (5 * perClass),
                        100.0 * escape / (4 * perClass), 100.0 * falseReject / perClass);
        }
    }
};

QTEST_GUILESS_MAIN(TstInspector)
#include "tst_inspector.moc"
