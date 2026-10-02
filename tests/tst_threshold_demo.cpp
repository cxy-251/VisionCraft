#include "ThresholdDemo.h"

#include <QtTest>
#include <cstdio>

// 光照越不均，单一全局阈值分割得越差：用「均匀光照下大津法的结果」做参照，统计分错的像素比例
class TstThresholdDemo : public QObject {
    Q_OBJECT
private slots:
    void otsuUnderUnevenLighting()
    {
        ThresholdDemo d;
        d.useOtsu();
        const QImage reference = d.binary();
        const int otsu0 = d.otsu();
        QVERIFY(otsu0 > 80 && otsu0 < 170);   // 背景约 52、金属约 200，大津法应落在两者之间

        std::printf("\n光照不均  大津阈值  与均匀光照结果不同的像素\n");
        for (double l : {0.0, 0.25, 0.5, 0.75, 1.0}) {
            d.setLighting(l);
            d.useOtsu();
            const QImage b = d.binary();
            int diff = 0;
            for (int y = 0; y < b.height(); ++y)
                for (int x = 0; x < b.width(); ++x)
                    diff += (qGray(b.pixel(x, y)) != qGray(reference.pixel(x, y)));
            std::printf("%6.0f%%   %6d     %6.2f%%\n", l * 100, d.otsu(), 100.0 * diff / (b.width() * b.height()));
        }
    }
};

QTEST_GUILESS_MAIN(TstThresholdDemo)
#include "tst_threshold_demo.moc"
