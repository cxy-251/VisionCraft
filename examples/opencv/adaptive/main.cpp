// 自适应阈值：每个像素用自己周围的亮度来定阈值，对付光照不均
//
// 运行：./example_opencv_adaptive <输出目录>     输出见 output.txt
// 光照模型和手册「全局阈值与 OTSU」一节的交互演示是同一个函数（ThresholdDemo::applyLighting）。
// 参照答案：均匀光照下，模糊后用大津法分割的结果。统计分错的像素占整张图的比例。

#include "PartGenerator.h"
#include "ThresholdDemo.h"

#include <QDir>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static double wrongPercent(const cv::Mat &bin, const cv::Mat &truth)
{
    cv::Mat diff;
    cv::compare(bin, truth, diff, cv::CMP_NE);
    return 100.0 * cv::countNonZero(diff) / double(truth.total());
}

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);
    PartGenerator gen(20261003);
    cv::Mat part;
    cv::cvtColor(gen.make(VC_DEFECT_NONE, 0.0).image, part, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(part, part, cv::Size(5, 5), 0);
    cv::Mat truth;
    cv::threshold(part, truth, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    std::printf("光照不均程度   大津法   自适应 15   自适应 51   自适应 151   （分错的像素 %%）\n");
    for (double lighting : {0.0, 0.5, 1.0}) {
        const cv::Mat gray = ThresholdDemo::applyLighting(part, lighting);
        // [region compare]
        cv::Mat otsu;
        cv::threshold(gray, otsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);   // 全图一个阈值
        double wrong[3];
        cv::Mat adaptive[3];
        const int blocks[3] = {15, 51, 151};
        for (int i = 0; i < 3; ++i) {
            // 每个像素的阈值 = 它周围 block×block 区域的高斯加权平均 − C（这里 C = −5，即要比周围亮 5 才算前景）
            cv::adaptiveThreshold(gray, adaptive[i], 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, blocks[i], -5);
            wrong[i] = wrongPercent(adaptive[i], truth);
        }
        // [endregion]
        std::printf("%10.1f   %7.2f   %9.2f   %9.2f   %10.2f\n", lighting, wrongPercent(otsu, truth), wrong[0], wrong[1], wrong[2]);
        if (lighting == 1.0) {
            cv::imwrite((dir + "/adaptive-input.png").toStdString(), gray);
            cv::imwrite((dir + "/adaptive-otsu.png").toStdString(), otsu);
            cv::imwrite((dir + "/adaptive-15.png").toStdString(), adaptive[0]);
            cv::imwrite((dir + "/adaptive-151.png").toStdString(), adaptive[2]);
        }
    }
    return 0;
}
