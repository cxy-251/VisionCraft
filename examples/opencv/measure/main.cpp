// 标准样差分、卡尺测量、距离变换与骨架、分水岭
//
// 运行：./example_opencv_measure <输出目录>     输出见 output.txt

#include "PartGenerator.h"

#include <QDir>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *n, const cv::Mat &m) { cv::imwrite((g_dir + "/" + n + ".png").toStdString(), m); }

static cv::Point2f washerCenter(const cv::Mat &gray)
{
    cv::Mat b, bin;
    cv::GaussianBlur(gray, b, cv::Size(5, 5), 0);
    cv::threshold(b, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    std::vector<std::vector<cv::Point>> cs;
    cv::findContours(bin, cs, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    std::vector<cv::Point> hull;
    cv::convexHull(*std::max_element(cs.begin(), cs.end(), [](auto &a, auto &c) { return a.size() < c.size(); }), hull);
    return cv::fitEllipse(hull).center;
}

// 画一个外径、内径已知（可以是小数）的理想垫圈：在 8 倍分辨率上画，再缩小，边缘是真实的部分覆盖灰度
static cv::Mat washer(double R, double r)
{
    const int S = 8;
    cv::Mat big(360 * S, 480 * S, CV_8UC1, cv::Scalar(52));
    const cv::Point c(240 * S, 180 * S);
    cv::circle(big, c, int(std::lround(R * S)), cv::Scalar(196), cv::FILLED);
    cv::circle(big, c, int(std::lround(r * S)), cv::Scalar(52), cv::FILLED);
    cv::Mat img;
    cv::resize(big, img, cv::Size(480, 360), 0, 0, cv::INTER_AREA);
    cv::Mat noise(img.size(), CV_16SC1);
    cv::RNG(11).fill(noise, cv::RNG::NORMAL, 0, 3);
    cv::add(img, noise, img, cv::noArray(), CV_8U);
    return img;
}

// [region caliper]
// 卡尺：沿一条线取亮度剖面，找亮度变化最大的位置。返回 (整数像素位置, 亚像素位置)
static std::pair<int, double> edgeAlong(const cv::Mat &gray, cv::Point2f from, cv::Point2f dir, int length, bool rising)
{
    std::vector<double> profile(length);
    for (int i = 0; i < length; ++i) {
        cv::Mat px;
        cv::getRectSubPix(gray, {1, 1}, from + dir * float(i), px, CV_32F);   // 双线性取样，位置可以是小数
        profile[i] = px.at<float>(0);
    }
    int best = 1;
    double bestG = 0;
    for (int i = 1; i + 1 < length; ++i) {
        const double g = (profile[i + 1] - profile[i - 1]) / 2 * (rising ? 1 : -1);   // 中心差分求导
        if (g > bestG) { bestG = g; best = i; }
    }
    // 用导数最大处和左右两点拟合一条抛物线，顶点就是亚像素的边缘位置
    auto g = [&](int i) { return (profile[i + 1] - profile[i - 1]) / 2 * (rising ? 1 : -1); };
    const double gl = g(best - 1), gc = g(best), gr = g(best + 1);
    const double offset = 0.5 * (gl - gr) / (gl - 2 * gc + gr);
    return {best, best + offset};
}
// [endregion]

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);

    std::printf("==== 1. 标准样差分 ====\n");
    // [region golden]
    PartGenerator gen(20261005);
    cv::Mat golden, test;
    cv::cvtColor(gen.make(VC_DEFECT_NONE, 0.0).image, golden, cv::COLOR_BGR2GRAY);     // 标准样：一件合格品
    cv::cvtColor(gen.make(VC_DEFECT_SPOT, 0.0).image, test, cv::COLOR_BGR2GRAY);       // 待检件：有一个污点
    auto bigDiff = [&](const cv::Mat &a, const cv::Mat &b, const char *name) {
        cv::Mat d, mask;
        cv::absdiff(a, b, d);
        cv::threshold(d, mask, 40, 255, cv::THRESH_BINARY);      // 差异超过 40 算「不一样」
        save(name, mask);
        return cv::countNonZero(mask);
    };
    std::printf("  直接相减：差异 > 40 的像素 %d 个\n", bigDiff(golden, test, "golden-raw"));
    const cv::Point2f shift = washerCenter(golden) - washerCenter(test);        // 两件的位置差
    cv::Mat aligned;
    const cv::Mat T = (cv::Mat_<double>(2, 3) << 1, 0, shift.x, 0, 1, shift.y);   // 纯平移
    cv::warpAffine(test, aligned, T, test.size(),
                   cv::INTER_LINEAR, cv::BORDER_REPLICATE);
    std::printf("  按圆心平移 (%.1f, %.1f) 对齐后再相减：%d 个\n", shift.x, shift.y, bigDiff(golden, aligned, "golden-aligned"));
    {   // 剩下的差异在哪：传送带上、垫圈边缘附近、还是环面中间
        cv::Mat d, mask;
        cv::absdiff(golden, aligned, d);
        cv::threshold(d, mask, 40, 255, cv::THRESH_BINARY);
        const cv::Point2f c = washerCenter(golden);
        int belt = 0, rim = 0, ringMid = 0;
        for (int y = 0; y < mask.rows; ++y)
            for (int x = 0; x < mask.cols; ++x) {
                if (!mask.at<uchar>(y, x)) continue;
                const double r = cv::norm(cv::Point2f(float(x), float(y)) - c);
                if (r > 122 || r < 40) ++belt;                       // 外圆以外或内孔里：看到的是传送带
                else if (r > 106 || r < 56) ++rim;                   // 离外缘、内孔边缘 8 像素以内
                else ++ringMid;
            }
        std::printf("    其中传送带上 %d、垫圈边缘附近 %d、环面中间 %d\n", belt, rim, ringMid);
    }
    // [endregion]

    std::printf("\n==== 2. 卡尺测量：环宽（外径 − 内径） ====\n");
    std::printf("  真实环宽     整数像素     亚像素\n");
    for (double width : {60.0, 60.25, 60.5, 60.75, 61.0}) {
        const double R = 110, r = R - width;
        const cv::Mat img = washer(R, r);
        // 从圆心沿 +x 方向：先遇到内孔边缘（暗→亮），再遇到外缘（亮→暗）
        const auto inner = edgeAlong(img, {240, 180}, {1, 0}, 80, true);
        const auto outer = edgeAlong(img, {300, 180}, {1, 0}, 80, false);
        std::printf("  %8.2f   %10d   %10.2f\n", width, (60 + outer.first) - inner.first, (60 + outer.second) - inner.second);
    }

    std::printf("\n==== 3. 距离变换与骨架 ====\n");
    // [region distance]
    cv::Mat ring = washer(110, 50) > 124;                         // 环宽 60 的垫圈，二值化
    cv::Mat dist;
    cv::distanceTransform(ring, dist, cv::DIST_L2, 5);           // 每个白点到最近黑点的距离
    double maxD;
    cv::minMaxLoc(dist, nullptr, &maxD);
    std::printf("  距离变换的最大值 %.1f → 环宽约 %.1f（真实 60）\n", maxD, 2 * maxD);
    // [endregion]
    // [region skeleton]
    // 形态学骨架：反复腐蚀，每次把「腐蚀后开运算会丢掉的部分」攒起来
    cv::Mat img = ring.clone(), skel(ring.size(), CV_8UC1, cv::Scalar(0)), eroded, opened;
    const cv::Mat k = cv::getStructuringElement(cv::MORPH_CROSS, {3, 3});
    int rounds = 0;
    while (cv::countNonZero(img) > 0) {
        cv::erode(img, eroded, k);
        cv::dilate(eroded, opened, k);
        skel |= img - opened;
        eroded.copyTo(img);          // 必须复制：写成 img = eroded，两者共用一块内存，下一轮 erode 就会原地改掉 img
        ++rounds;
    }
    std::printf("  骨架：腐蚀了 %d 轮，骨架像素 %d 个（中线圆周长 2π×80 ≈ %.0f）\n", rounds, cv::countNonZero(skel), 2 * CV_PI * 80);
    // [endregion]
    // [region skeleton-bug]
    {   // 错误写法：img = eroded（共享内存）
        cv::Mat img2 = ring.clone(), skel2(ring.size(), CV_8UC1, cv::Scalar(0)), er, op;
        int r2 = 0;
        while (cv::countNonZero(img2) > 0 && r2 < 200) {
            cv::erode(img2, er, k);
            cv::dilate(er, op, k);
            skel2 |= img2 - op;
            img2 = er;
            ++r2;
        }
        std::printf("  同样的循环写成 img = eroded：%d 轮，骨架像素 %d 个\n", r2, cv::countNonZero(skel2));
    }
    // [endregion]
    cv::Mat distShown;
    dist.convertTo(distShown, CV_8U, 255.0 / maxD);
    save("distance", distShown);
    save("skeleton", skel);

    std::printf("\n==== 4. 分水岭：分开两个挨在一起的垫圈 ====\n");
    // [region watershed]
    cv::Mat two(240, 400, CV_8UC1, cv::Scalar(0));
    cv::circle(two, {140, 120}, 80, cv::Scalar(255), cv::FILLED);
    cv::circle(two, {270, 120}, 70, cv::Scalar(255), cv::FILLED);   // 两个圆重叠了 20 像素
    cv::Mat labels;
    std::printf("  直接数连通域：%d 个\n", cv::connectedComponents(two, labels) - 1);
    cv::Mat d2;
    cv::distanceTransform(two, d2, cv::DIST_L2, 5);
    cv::Mat seeds = d2 > 0.6 * 70;                                  // 离边缘很远的点：每个圆的「核」
    cv::Mat markers;
    const int nSeeds = cv::connectedComponents(seeds, markers, 8, CV_32S) - 1;
    markers.setTo(nSeeds + 1, two == 0);                            // 背景也给一个编号
    cv::Mat color;
    cv::cvtColor(two, color, cv::COLOR_GRAY2BGR);
    cv::watershed(color, markers);                                  // 从种子出发「灌水」，相遇处是分界线，值为 −1
    const int lineInside = cv::countNonZero((markers == -1) & (two > 0));
    std::printf("  种子 %d 个；分水岭标成 −1 的像素 %d 个，其中落在白色区域里的 %d 个\n", nSeeds, cv::countNonZero(markers == -1), lineInside);
    for (int i = 1; i <= nSeeds; ++i)
        std::printf("  第 %d 块面积 %d\n", i, cv::countNonZero(markers == i));
    // [endregion]
    cv::Mat shown(two.size(), CV_8UC3, cv::Scalar(0, 0, 0));
    shown.setTo(cv::Scalar(90, 210, 90), markers == 1);
    shown.setTo(cv::Scalar(240, 200, 40), markers == 2);
    shown.setTo(cv::Scalar(60, 60, 240), markers == -1);
    save("watershed", shown);
    std::printf("  两块 + 白色区域里的分界线 = %d；原图白色像素 %d\n", cv::countNonZero(markers == 1) + cv::countNonZero(markers == 2) + lineInside, cv::countNonZero(two));
    return 0;
}
