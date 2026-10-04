// 方框滤波、双边滤波、Laplacian：耗时随窗口怎么变、参数怎么影响结果、二阶导数对噪声多敏感
//
// 运行：./example_opencv_filters2 <输出目录>     输出见 output.txt（耗时每次运行不同）

#include "PartGenerator.h"

#include <QDir>
#include <QElapsedTimer>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

template <typename F>
static double timeUs(F f, int n = 20)
{
    f();                      // 先跑一次「热身」：第一次调用要分配内存、初始化线程池，不计时
    QElapsedTimer t;
    t.start();
    for (int i = 0; i < n; ++i)
        f();
    return t.nsecsElapsed() / 1e3 / n;
}

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);
    PartGenerator gen(20261003);
    cv::Mat gray;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, gray, cv::COLOR_BGR2GRAY);

    std::printf("==== 1. 窗口越大越慢吗（480×360 灰度图，单位 µs） ====\n");
    std::printf("  窗口      blur     GaussianBlur   medianBlur\n");
    // [region cost]
    for (int k : {3, 5, 7, 9, 31, 61}) {
        cv::Mat out;
        const double box = timeUs([&] { cv::blur(gray, out, cv::Size(k, k)); });
        const double gauss = timeUs([&] { cv::GaussianBlur(gray, out, cv::Size(k, k), 0); });
        const double median = timeUs([&] { cv::medianBlur(gray, out, k); });
        std::printf("  %3d×%-3d %7.0f   %10.0f   %10.0f\n", k, k, box, gauss, median);
    }
    // [endregion]

    std::printf("\n==== 2. boxFilter 不归一化：窗口里的和 ====\n");
    // [region sum]
    cv::Mat ones(5, 5, CV_8UC1, cv::Scalar(10)), sum;
    cv::boxFilter(ones, sum, CV_32S, cv::Size(3, 3), cv::Point(-1, -1), false);   // normalize = false
    std::printf("  全是 10 的图，3×3 窗口求和：中心 %d，角上 %d（边界按镜像补齐）\n", sum.at<int>(2, 2), sum.at<int>(0, 0));
    // [endregion]

    std::printf("\n==== 3. 双边滤波的 sigmaColor：多大的亮度差算「另一边」 ====\n");
    // [region bilateral]
    cv::Mat step(120, 200, CV_8UC1, cv::Scalar(80));
    step.colRange(100, 200).setTo(130);                 // 台阶高 50
    cv::Mat noise(step.size(), CV_16SC1), noisy;
    cv::RNG(3).fill(noise, cv::RNG::NORMAL, 0, 10);
    cv::add(step, noise, noisy, cv::noArray(), CV_8U);
    for (double sc : {10.0, 30.0, 75.0, 200.0}) {
        cv::Mat out;
        cv::bilateralFilter(noisy, out, 9, sc, 9);
        cv::Mat flat = out.colRange(10, 90), row;
        cv::Scalar m, sd;
        cv::meanStdDev(flat, m, sd);
        cv::reduce(out, row, 0, cv::REDUCE_AVG, CV_64F);
        std::printf("  sigmaColor %5.0f：平坦处噪声标准差 %4.1f，台阶两侧（第 98/101 列）%.0f → %.0f\n", sc, sd[0],
                    row.at<double>(0, 98), row.at<double>(0, 101));
        if (sc == 30.0) cv::imwrite((dir + "/bilateral-30.png").toStdString(), out);
        if (sc == 200.0) cv::imwrite((dir + "/bilateral-200.png").toStdString(), out);
    }
    cv::imwrite((dir + "/bilateral-input.png").toStdString(), noisy);
    // [endregion]

    std::printf("\n==== 4. Laplacian：二阶导数，对噪声比一阶导数敏感 ====\n");
    // [region laplacian]
    for (int pre : {0, 5}) {
        cv::Mat src = gray, sob, lap, sx, sy;
        if (pre) cv::GaussianBlur(gray, src, cv::Size(pre, pre), 0);
        cv::Laplacian(src, lap, CV_16S, 3);
        cv::Sobel(src, sx, CV_16S, 1, 0, 3);
        cv::Sobel(src, sy, CV_16S, 0, 1, 3);
        // 在传送带上一块没有任何物体的区域，量两种导数的「噪声幅度」
        const cv::Rect bg(10, 10, 80, 80);
        cv::Scalar m, sdLap, sdSob;
        cv::meanStdDev(lap(bg), m, sdLap);
        cv::Mat sxf;
        sx(bg).convertTo(sxf, CV_32F);
        cv::meanStdDev(sxf, m, sdSob);
        double lapMax, sobMax;
        cv::minMaxLoc(cv::abs(lap), nullptr, &lapMax);
        cv::minMaxLoc(cv::abs(sx), nullptr, &sobMax);
        std::printf("  %s：背景处标准差 Laplacian %.1f / Sobel-x %.1f；全图最大值 Laplacian %.0f / Sobel-x %.0f\n",
                    pre ? "先高斯 5×5" : "不模糊    ", sdLap[0], sdSob[0], lapMax, sobMax);
        if (!pre) {
            cv::Mat shown;
            lap.convertTo(shown, CV_8U, 0.5, 128);
            cv::imwrite((dir + "/laplacian.png").toStdString(), shown);
        }
    }
    // [endregion]
    return 0;
}
