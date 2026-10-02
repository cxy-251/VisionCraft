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
};

QTEST_GUILESS_MAIN(TstInspector)
#include "tst_inspector.moc"
