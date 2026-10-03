// 模拟产线的「难度」：同一种缺陷在难度 0、0.5、1 下长什么样，检测算法各判成什么
//
// 运行：./example_opencv_part_gallery <输出目录>     输出见 output.txt
// 每张图只截工件附近 280×280 的区域，存 JPEG。

#include "Inspector.h"
#include "PartGenerator.h"

#include <QDir>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);
    PartGenerator gen(20261004);
    Inspector insp;
    const struct { vc_defect d; const char *key; } kinds[] = {
        {VC_DEFECT_SCRATCH, "scratch"}, {VC_DEFECT_SPOT, "spot"}, {VC_DEFECT_CHIP, "chip"}, {VC_DEFECT_OFFSET, "offset"}};
    std::printf("缺陷      难度 0          难度 0.5        难度 1\n");
    for (const auto &k : kinds) {
        std::printf("%-8s", qPrintable(Inspector::defectName(k.d)));
        for (double difficulty : {0.0, 0.5, 1.0}) {
            const PartGenerator::Part p = gen.make(k.d, difficulty);
            const Inspector::Result r = insp.inspect(p.image);
            std::printf("  判为 %-10s", qPrintable(Inspector::defectName(r.defect)));
            const cv::Rect crop(100, 40, 280, 280);
            cv::imwrite(QStringLiteral("%1/gallery-%2-%3.jpg").arg(dir, k.key).arg(int(difficulty * 10)).toStdString(),
                        p.image(crop), {cv::IMWRITE_JPEG_QUALITY, 88});
        }
        std::printf("\n");
    }
    return 0;
}
