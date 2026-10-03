// 滤波：均值、高斯、中值、双边——对付两种噪声各自的效果和代价
//
// 运行：./example_opencv_filters <输出目录>     输出见 output.txt

#include "PartGenerator.h"

#include <QDir>
#include <QElapsedTimer>
#include <QString>
#include <algorithm>
#include <cstdio>
#include <functional>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *name, const cv::Mat &m) { cv::imwrite((g_dir + "/" + name + ".png").toStdString(), m); }

// 二值化后的「碎块」数：理想情况只有垫圈一块
static int blobs(const cv::Mat &gray)
{
    cv::Mat bin, labels;
    cv::threshold(gray, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    return cv::connectedComponents(bin, labels, 8) - 1;
}

// 边缘有多陡：把每一列的亮度按行平均（压掉噪声），看从 10% 走到 90% 亮度用了几个像素
static int edgeWidth(const cv::Mat &gray, int x0, int x1)
{
    cv::Mat line;
    cv::reduce(gray.colRange(x0, x1), line, 0, cv::REDUCE_AVG, CV_64F);
    double lo, hi;
    cv::minMaxLoc(line, &lo, &hi);
    int a = -1, b = -1;
    for (int x = 0; x < line.cols; ++x) {
        const double v = line.at<double>(0, x);
        if (a < 0 && v > lo + 0.1 * (hi - lo)) a = x;
        if (b < 0 && v > lo + 0.9 * (hi - lo)) b = x;
    }
    return b - a;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);

    // [region inputs]
    // 一块干净的「台阶」：左边暗 60，右边亮 200，中间是一条竖直的边
    cv::Mat clean(160, 240, CV_8UC1, cv::Scalar(60));
    clean.colRange(120, 240).setTo(200);
    cv::RNG rng(7);
    // 高斯噪声：每个像素都偏一点，偏多少服从正态分布（标准差 25）
    cv::Mat noise(clean.size(), CV_16SC1);
    rng.fill(noise, cv::RNG::NORMAL, 0, 25);
    cv::Mat gaussNoisy;
    cv::add(clean, noise, gaussNoisy, cv::noArray(), CV_8U);
    // 椒盐噪声：5% 的像素随机变成纯黑或纯白，其余完全不动
    cv::Mat saltPepper = clean.clone();
    for (int i = 0; i < clean.total() * 5 / 100; ++i)
        saltPepper.at<uchar>(rng.uniform(0, clean.rows), rng.uniform(0, clean.cols)) = rng.uniform(0, 2) ? 255 : 0;
    // [endregion]

    // [region filters]
    const struct { const char *name, *key; std::function<void(const cv::Mat &, cv::Mat &)> f; } filters[] = {
        {"不滤波", "none", [](const cv::Mat &s, cv::Mat &d) { d = s.clone(); }},
        {"均值 5×5", "mean", [](const cv::Mat &s, cv::Mat &d) { cv::blur(s, d, cv::Size(5, 5)); }},
        {"高斯 5×5", "gauss", [](const cv::Mat &s, cv::Mat &d) { cv::GaussianBlur(s, d, cv::Size(5, 5), 0); }},
        {"中值 5", "median", [](const cv::Mat &s, cv::Mat &d) { cv::medianBlur(s, d, 5); }},
        {"双边 d=9", "bilateral", [](const cv::Mat &s, cv::Mat &d) { cv::bilateralFilter(s, d, 9, 75, 75); }},
    };
    // [endregion]

    for (const auto &[label, input, file] : {std::tuple{"高斯噪声（σ=25）", gaussNoisy, "gauss"},
                                              std::tuple{"椒盐噪声（5%）", saltPepper, "saltpepper"}}) {
        std::printf("==== %s ====\n", label);
        std::printf("  滤波方法      与干净图的平均误差  边缘宽度(像素)  耗时(µs)\n");
        save((std::string("filter-") + file + "-input").c_str(), input);
        for (const auto &flt : filters) {
            cv::Mat out;
            QElapsedTimer t;
            t.start();
            for (int i = 0; i < 20; ++i)
                flt.f(input, out);
            const double us = t.nsecsElapsed() / 1e3 / 20;
            const double err = cv::norm(out, clean, cv::NORM_L1) / clean.total();
            std::printf("  %-12s %12.1f %14d %12.0f\n", flt.name, err, edgeWidth(out, 100, 140), us);
            if (std::string(flt.key) != "none")
                save((std::string("filter-") + file + "-" + flt.key).c_str(), out);
        }
    }

    std::printf("\n==== 工位为什么先模糊再大津：合格工件，二值化后外轮廓有多毛糙 ====\n");
    // [region otsu]
    PartGenerator gen(20261003);
    cv::Mat gray;
    cv::cvtColor(gen.make(VC_DEFECT_NONE, 0.0).image, gray, cv::COLOR_BGR2GRAY);
    for (int k : {1, 3, 5, 9}) {
        cv::Mat b = gray.clone(), bin;
        if (k > 1)
            cv::GaussianBlur(gray, b, cv::Size(k, k), 0);
        cv::threshold(b, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(bin, contours, cv::RETR_CCOMP, cv::CHAIN_APPROX_NONE);
        const auto &outer = *std::max_element(contours.begin(), contours.end(),
            [](const auto &x, const auto &y) { return cv::contourArea(x) < cv::contourArea(y); });
        std::vector<int> hull;
        cv::convexHull(outer, hull, false, false);
        std::sort(hull.begin(), hull.end());
        std::vector<cv::Vec4i> defects;
        cv::convexityDefects(outer, hull, defects);
        double deepest = 0;
        for (const auto &d : defects) deepest = std::max(deepest, d[3] / 256.0);
        std::printf("  %s：轮廓 %zu 条，外轮廓 %zu 个点，最深凸缺陷 %.1f 像素\n",
                    k > 1 ? qPrintable(QStringLiteral("高斯 %1×%1").arg(k)) : "不模糊", contours.size(), outer.size(), deepest);
    }
    // [endregion]

    std::printf("\n==== 模糊对表面检测（黑帽 21×21，阈值 18）的影响：合格件和划痕件 ====\n");
    // [region blackhat]
    for (vc_defect d : {VC_DEFECT_NONE, VC_DEFECT_SCRATCH}) {
        cv::Mat g;
        cv::cvtColor(gen.make(d, 0.8).image, g, cv::COLOR_BGR2GRAY);   // 难度 0.8：划痕比较淡
        for (int k : {1, 5}) {
            cv::Mat b = g.clone(), bh, suspect, labels, stats, cent;
            if (k > 1)
                cv::GaussianBlur(g, b, cv::Size(k, k), 0);
            cv::morphologyEx(b, bh, cv::MORPH_BLACKHAT, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21)));
            cv::threshold(bh, suspect, 18, 255, cv::THRESH_BINARY);
            const int n = cv::connectedComponentsWithStats(suspect, labels, stats, cent, 8);
            int big = 0, largest = 0;
            for (int i = 1; i < n; ++i) {
                const int a = stats.at<int>(i, cv::CC_STAT_AREA);
                largest = std::max(largest, a);
                if (a >= 12) ++big;
            }
            std::printf("  %s %s：超过阈值 %5d 个像素，%4d 块，其中 ≥12 像素的 %d 块（最大 %d）\n",
                        d == VC_DEFECT_NONE ? "合格件" : "划痕件", k > 1 ? "先高斯 5×5" : "不模糊    ",
                        cv::countNonZero(suspect), n - 1, big, largest);
        }
    }
    // [endregion]
    return 0;
}
