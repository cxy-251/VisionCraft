// 连通域：把二值图里连在一起的白色像素分成一块一块，并统计每块的面积、外框、中心
//
// 运行：./example_opencv_components <输出目录>     输出见 output.txt

#include "Inspector.h"
#include "PartGenerator.h"

#include <QDir>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static void listComponents(const cv::Mat &bin, int connectivity)
{
    // [region stats]
    cv::Mat labels, stats, centroids;
    const int n = cv::connectedComponentsWithStats(bin, labels, stats, centroids, connectivity);
    // 第 0 号是背景。stats 每行：左、上、宽、高、面积
    std::printf("  %d 连通：%d 块\n", connectivity, n - 1);
    for (int i = 1; i < n && i <= 6; ++i)
        std::printf("    #%d 面积 %3d  外框 %3d×%-3d  中心 (%.1f, %.1f)\n", i, stats.at<int>(i, cv::CC_STAT_AREA),
                    stats.at<int>(i, cv::CC_STAT_WIDTH), stats.at<int>(i, cv::CC_STAT_HEIGHT),
                    centroids.at<double>(i, 0), centroids.at<double>(i, 1));
    if (n - 1 > 6)
        std::printf("    ……（共 %d 块）\n", n - 1);
    // [endregion]
}

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);

    std::printf("==== 1. 4 连通和 8 连通：一条 1 像素宽的斜线 ====\n");
    // [region diagonal]
    cv::Mat line(40, 40, CV_8UC1, cv::Scalar(0));
    cv::line(line, cv::Point(5, 5), cv::Point(34, 34), cv::Scalar(255), 1, cv::LINE_8);   // 斜线上相邻的点只在角上相接
    // [endregion]
    listComponents(line, 4);
    listComponents(line, 8);

    std::printf("\n==== 2. 工位检测里的划痕和污点 ====\n");
    PartGenerator gen(20261003);
    Inspector insp;
    for (vc_defect d : {VC_DEFECT_SCRATCH, VC_DEFECT_SPOT}) {
        Inspector::Steps steps;
        const Inspector::Result r = insp.inspect(gen.make(d, 0.0).image, &steps);
        const cv::Mat *suspect = nullptr;
        for (const auto &s : steps)
            if (s.first == QLatin1String("suspect"))
                suspect = &s.second;
        std::printf("%s（判定 %s，长宽比 %.1f）\n", qPrintable(Inspector::defectName(d)), qPrintable(Inspector::defectName(r.defect)),
                    r.measures.value(QStringLiteral("elongation")).toDouble());
        listComponents(*suspect, 4);
        listComponents(*suspect, 8);
        if (d == VC_DEFECT_SCRATCH)
            cv::imwrite((dir + QStringLiteral("/components-scratch.png")).toStdString(), *suspect);
    }
    return 0;
}
