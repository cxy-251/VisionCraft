// 混合与饱和运算、直方图均衡与 CLAHE、图像金字塔、HSV 颜色提取、FileStorage 存参数
//
// 运行：./example_opencv_basics2 <输出目录>     输出见 output.txt

#include "PartGenerator.h"
#include "ThresholdDemo.h"

#include <QDir>
#include <QFile>
#include <QString>
#include <cstdio>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *n, const cv::Mat &m) { cv::imwrite((g_dir + "/" + n + ".png").toStdString(), m); }

static double otsuWrong(const cv::Mat &gray, const cv::Mat &truth)
{
    cv::Mat bin, diff;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    cv::compare(bin, truth, diff, cv::CMP_NE);
    return 100.0 * cv::countNonZero(diff) / double(truth.total());
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);
    PartGenerator gen(20261003);
    cv::Mat part;
    cv::cvtColor(gen.make(VC_DEFECT_NONE, 0.0).image, part, cv::COLOR_BGR2GRAY);

    std::printf("==== 1. 饱和运算与混合 ====\n");
    // [region saturate]
    cv::Mat a(1, 1, CV_8UC1, cv::Scalar(200)), b(1, 1, CV_8UC1, cv::Scalar(100)), sum, diff;
    cv::add(a, b, sum);                     // 200 + 100 = 300，8 位装不下
    cv::subtract(b, a, diff);               // 100 − 200 = −100
    const uchar raw = uchar(200 + 100);     // 普通 C++ 的 8 位加法：取低 8 位
    std::printf("  cv::add 200+100 = %d，cv::subtract 100−200 = %d，C++ uchar(200+100) = %d\n",
                sum.at<uchar>(0), diff.at<uchar>(0), raw);
    // [endregion]
    // [region blend]
    cv::Mat other;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, other, cv::COLOR_BGR2GRAY);
    cv::Mat blended;
    cv::addWeighted(part, 0.7, other, 0.3, 0.0, blended);   // 0.7 × 第一张 + 0.3 × 第二张 + 0
    std::printf("  addWeighted 0.7/0.3：中心像素 %d × 0.7 + %d × 0.3 = %d\n", part.at<uchar>(180, 240),
                other.at<uchar>(180, 240), blended.at<uchar>(180, 240));
    save("blend", blended);
    // [endregion]

    std::printf("\n==== 2. 直方图均衡与 CLAHE ====\n");
    cv::Mat truth;
    {
        cv::Mat b5;
        cv::GaussianBlur(part, b5, cv::Size(5, 5), 0);
        cv::threshold(b5, truth, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    }
    // [region equalize]
    cv::Mat dim;
    part.convertTo(dim, -1, 0.3, 20);       // 模拟曝光不足：亮度压到 0.3 倍再加 20
    cv::Mat eq;
    cv::equalizeHist(dim, eq);
    double mn, mx, mn2, mx2;
    cv::minMaxLoc(dim, &mn, &mx);
    cv::minMaxLoc(eq, &mn2, &mx2);
    std::printf("  曝光不足的图：亮度范围 %.0f~%.0f；equalizeHist 之后 %.0f~%.0f\n", mn, mx, mn2, mx2);
    save("eq-dim", dim);
    save("eq-global", eq);
    // [endregion]
    // [region eq-otsu]
    cv::Mat binDim, binEq;
    const double tDim = cv::threshold(dim, binDim, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    const double tEq = cv::threshold(eq, binEq, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    std::printf("  大津法直接分割曝光不足的图：阈值 %.0f，分错 %.1f%%；先均衡化再分割：阈值 %.0f，分错 %.1f%%\n",
                tDim, otsuWrong(dim, truth), tEq, otsuWrong(eq, truth));
    save("eq-otsu", binEq);
    // [endregion]
    // [region clahe]
    cv::Mat blurredPart;
    cv::GaussianBlur(part, blurredPart, cv::Size(5, 5), 0);
    const cv::Mat uneven = ThresholdDemo::applyLighting(blurredPart, 1.0);   // 和「自适应阈值」一节同样的光照
    cv::Mat eqU, claheU;
    cv::equalizeHist(uneven, eqU);
    cv::createCLAHE(2.0, cv::Size(8, 8))->apply(uneven, claheU);              // 分成 8×8 块，各自均衡，限制对比度 2
    std::printf("  光照不均 1.0，大津法分错：原图 %.1f%%，全局均衡后 %.1f%%，CLAHE 后 %.1f%%\n",
                otsuWrong(uneven, truth), otsuWrong(eqU, truth), otsuWrong(claheU, truth));
    save("clahe-input", uneven);
    save("clahe-global", eqU);
    save("clahe-clahe", claheU);
    // [endregion]

    std::printf("\n==== 3. 图像金字塔 ====\n");
    // [region pyramid]
    cv::Mat down, up;
    cv::pyrDown(part, down);                // 先高斯模糊，再隔一行一列取一个点：尺寸减半
    cv::pyrUp(down, up, part.size());       // 放大回原尺寸（插值），细节回不来
    cv::Mat lap;                            // 拉普拉斯金字塔的一层：原图 − 放大回来的图 = 丢掉的细节
    cv::subtract(part, up, lap, cv::noArray(), CV_16S);
    cv::Mat restored;
    cv::add(up, lap, restored, cv::noArray(), CV_8U);
    std::printf("  原图 %dx%d → pyrDown %dx%d → pyrUp %dx%d\n", part.cols, part.rows, down.cols, down.rows, up.cols, up.rows);
    std::printf("  pyrUp 回来和原图的平均差 %.2f；加上存下来的细节层之后 %.2f\n",
                cv::norm(up, part, cv::NORM_L1) / part.total(), cv::norm(restored, part, cv::NORM_L1) / part.total());
    // [endregion]
    // [region alias]
    // 传送带的横条纹周期约 18 行（sin(y × 0.35)）。缩到 1/16 后每个点代表 16 行，条纹已经细到表示不出来。
    // 直接每隔 16 行取一行（不先模糊）vs pyrDown 四次（每次先模糊）：看左边传送带那一列剩下什么
    cv::Mat naive, pyr = part.clone();
    cv::resize(part, naive, cv::Size(), 1.0 / 16, 1.0 / 16, cv::INTER_NEAREST);
    for (int i = 0; i < 4; ++i)
        cv::pyrDown(pyr, pyr);
    auto column = [](const cv::Mat &m) {
        QString s;
        for (int y = 0; y < m.rows; ++y)
            s += QString::number(m.at<uchar>(y, 1)) + ' ';
        return s;
    };
    std::printf("  缩到 1/16 后传送带那一列（从上到下）：\n    直接取点：%s\n    pyrDown×4：%s\n", qPrintable(column(naive)), qPrintable(column(pyr)));
    // [endregion]

    std::printf("\n==== 4. HSV 颜色提取：红色在色相环的两头 ====\n");
    // [region hsv]
    cv::Mat swatches(60, 360, CV_8UC3);
    const cv::Scalar colors[] = {{0, 0, 230}, {30, 30, 200}, {0, 200, 0}, {200, 0, 0}, {60, 20, 210}, {0, 0, 180}};   // BGR
    for (int i = 0; i < 6; ++i)
        swatches.colRange(i * 60, i * 60 + 60).setTo(colors[i]);
    cv::Mat hsv;
    cv::cvtColor(swatches, hsv, cv::COLOR_BGR2HSV);
    for (int i = 0; i < 6; ++i) {
        const cv::Vec3b p = hsv.at<cv::Vec3b>(30, i * 60 + 30);
        std::printf("  色块 %d：BGR(%3.0f,%3.0f,%3.0f) → H=%3d S=%3d V=%3d\n", i, colors[i][0], colors[i][1], colors[i][2], p[0], p[1], p[2]);
    }
    cv::Mat lowRed, highRed, red;
    cv::inRange(hsv, cv::Scalar(0, 100, 80), cv::Scalar(10, 255, 255), lowRed);     // H 0~10
    cv::inRange(hsv, cv::Scalar(170, 100, 80), cv::Scalar(179, 255, 255), highRed); // H 170~179
    cv::bitwise_or(lowRed, highRed, red);
    std::printf("  只取 H 0~10：%d 个色块；两段合起来：%d 个色块\n", cv::countNonZero(lowRed) / 3600, cv::countNonZero(red) / 3600);
    save("hsv-swatches", swatches);
    // [endregion]

    std::printf("\n==== 5. FileStorage：把参数存成 YAML 再读回来 ====\n");
    // [region storage]
    const QString file = g_dir + "/params.yml";
    {
        cv::FileStorage fs(file.toStdString(), cv::FileStorage::WRITE);
        fs << "surfaceThreshold" << 18 << "maxCenterOffset" << 6.0 << "kernel" << cv::Mat(cv::Mat::eye(2, 2, CV_32F));
    }
    QFile f(file);
    f.open(QIODevice::ReadOnly);
    std::printf("%s", f.readAll().constData());
    cv::FileStorage fs(file.toStdString(), cv::FileStorage::READ);
    int thr = fs["surfaceThreshold"];
    double off = fs["maxCenterOffset"];
    int missing = fs["minDefectArea"];       // 文件里没有这个键
    std::printf("  读回：surfaceThreshold=%d maxCenterOffset=%.1f；不存在的 minDefectArea=%d（isNone=%s）\n", thr, off, missing,
                fs["minDefectArea"].isNone() ? "true" : "false");
    // [endregion]
    QFile::remove(file);
    return 0;
}
