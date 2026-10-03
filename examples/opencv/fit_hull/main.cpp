// 外接圆、拟合椭圆、凸包与凸缺陷：外圆上有一个缺口时，圆心还量得准吗？
//
// 运行：./example_opencv_fit_hull <输出目录>     输出见 output.txt
// 真实圆心在 (160, 120)，半径 100。在外圆右侧挖一个深度不同的缺口，比较三种求圆心的方法。

#include <QDir>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static const cv::Point2f kCenter(160, 120);

// [region part]
static cv::Mat makePart(int chipDepth, int burr = 0)
{
    cv::Mat bin(240, 320, CV_8UC1, cv::Scalar(0));
    cv::circle(bin, cv::Point(160, 120), 100, cv::Scalar(255), cv::FILLED);
    cv::circle(bin, cv::Point(160, 120), 40, cv::Scalar(0), cv::FILLED);
    if (chipDepth > 0)   // 缺口：从外圆右边缘往里挖，宽 40 像素
        cv::rectangle(bin, cv::Rect(260 - chipDepth, 100, chipDepth + 1, 40), cv::Scalar(0), cv::FILLED);
    if (burr > 0)        // 毛刺：外圆左上方凸出去一小截，宽 4 像素
        cv::rectangle(bin, cv::Rect(84, 52 - burr, 4, burr + 8), cv::Scalar(255), cv::FILLED);
    return bin;
}
// [endregion]

static std::vector<cv::Point> outerContour(const cv::Mat &bin)
{
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    return *std::max_element(contours.begin(), contours.end(),
                             [](const auto &a, const auto &b) { return cv::contourArea(a) < cv::contourArea(b); });
}

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);

    std::printf("==== 圆心误差（像素）：真实圆心 (160,120) ====\n");
    std::printf("缺口深度  最小外接圆  直接拟合椭圆  凸包拟合椭圆  最深凸缺陷\n");
    const struct { int depth, burr; } cases[] = {{0, 0}, {5, 0}, {10, 0}, {20, 0}, {30, 0}, {40, 0}, {0, 8}, {0, 16}};
    for (const auto &k : cases) {
        const int depth = k.depth;
        if (k.burr && depth == 0 && k.burr == 8)
            std::printf("—— 没有缺口，外圆上有一根向外凸出的毛刺（长度见第一列，单位像素）——\n");
        const std::vector<cv::Point> c = outerContour(makePart(depth, k.burr));
        // [region methods]
        cv::Point2f encCenter;
        float encR;
        cv::minEnclosingCircle(c, encCenter, encR);              // 包住所有点的最小圆
        const cv::RotatedRect direct = cv::fitEllipse(c);         // 用全部轮廓点做最小二乘拟合
        std::vector<cv::Point> hull;
        cv::convexHull(c, hull);                                  // 凸包：像橡皮筋箍住轮廓，凹进去的地方直接跨过
        const cv::RotatedRect viaHull = cv::fitEllipse(hull);
        // [endregion]

        // [region defects]
        std::vector<int> hullIdx;
        cv::convexHull(c, hullIdx, false, false);                 // 这次要的是凸包点在轮廓里的下标
        std::vector<cv::Vec4i> defects;                           // 每个：[起点, 终点, 最深点, 深度×256]
        cv::convexityDefects(c, hullIdx, defects);
        double deepest = 0;
        for (const cv::Vec4i &d : defects)
            deepest = std::max(deepest, d[3] / 256.0);
        // [endregion]

        std::printf("%6d  %10.2f  %12.2f  %12.2f  %10.1f\n", k.burr ? k.burr : depth, cv::norm(encCenter - kCenter),
                    cv::norm(direct.center - kCenter), cv::norm(viaHull.center - kCenter), deepest);

        if (depth == 30) {   // 配图
            cv::Mat shown;
            cv::cvtColor(makePart(depth) / 3, shown, cv::COLOR_GRAY2BGR);
            cv::polylines(shown, hull, true, cv::Scalar(240, 200, 40), 1, cv::LINE_AA);
            cv::ellipse(shown, direct, cv::Scalar(60, 60, 240), 1, cv::LINE_AA);
            cv::ellipse(shown, viaHull, cv::Scalar(90, 210, 90), 1, cv::LINE_AA);
            cv::drawMarker(shown, direct.center, cv::Scalar(60, 60, 240), cv::MARKER_CROSS, 12, 2);
            cv::drawMarker(shown, viaHull.center, cv::Scalar(90, 210, 90), cv::MARKER_CROSS, 12, 2);
            for (const cv::Vec4i &d : defects)
                if (d[3] / 256.0 > 5)
                    cv::circle(shown, c[d[2]], 4, cv::Scalar(60, 60, 240), cv::FILLED);
            cv::imwrite((dir + QStringLiteral("/fit-chip30.png")).toStdString(), shown);
        }
    }
    return 0;
}
