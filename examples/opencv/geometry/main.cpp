// 霍夫圆、霍夫直线、仿射变换、透视变换
//
// 运行：./example_opencv_geometry <输出目录>     输出见 output.txt

#include "PartGenerator.h"

#include <QDir>
#include <QElapsedTimer>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *n, const cv::Mat &m) { cv::imwrite((g_dir + "/" + n + ".png").toStdString(), m); }

// 画一个圆心、半径已知的垫圈（和模拟产线同样的颜色和噪声），可选一个缺口
static cv::Mat washer(cv::Point2d c, double R, double r, bool chip)
{
    cv::Mat img(360, 480, CV_8UC1, cv::Scalar(52));
    cv::circle(img, c, int(R), cv::Scalar(196), cv::FILLED, cv::LINE_AA);
    cv::circle(img, c, int(r), cv::Scalar(52), cv::FILLED, cv::LINE_AA);
    if (chip) cv::circle(img, c + cv::Point2d(R, 0), 18, cv::Scalar(52), cv::FILLED, cv::LINE_AA);
    cv::Mat noise(img.size(), CV_16SC1);
    cv::RNG(5).fill(noise, cv::RNG::NORMAL, 0, 4);
    cv::add(img, noise, img, cv::noArray(), CV_8U);
    return img;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);

    std::printf("==== 1. 霍夫圆 vs 轮廓拟合：真实外圆圆心 (243.0, 178.0)、半径 115 ====\n");
    for (bool chip : {false, true}) {
        const cv::Point2d truth(243, 178);
        const cv::Mat img = washer(truth, 115, 46, chip);
        // [region hough-circle]
        cv::Mat blurred;
        cv::GaussianBlur(img, blurred, cv::Size(5, 5), 0);
        std::vector<cv::Vec3f> circles;   // 每个：(x, y, 半径)
        cv::HoughCircles(blurred, circles, cv::HOUGH_GRADIENT, 1, 50, 100, 30, 90, 140);   // 热身，不计时
        QElapsedTimer t;
        t.start();
        cv::HoughCircles(blurred, circles, cv::HOUGH_GRADIENT, 1, 50, 100, 30, 90, 140);
        const double houghMs = t.nsecsElapsed() / 1e6;
        //                                       dp minDist 边缘阈值 累加阈值 半径范围
        // [endregion]
        // 对照：工位的做法（大津 → 轮廓 → 凸包 → 拟合椭圆）
        cv::RotatedRect fit;
        double fitMs = 0;
        for (int warm = 0; warm < 2; ++warm) {   // 第一遍热身，第二遍计时
        t.restart();
        cv::Mat bin;
        cv::threshold(blurred, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        std::vector<std::vector<cv::Point>> cs;
        cv::findContours(bin, cs, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
        std::vector<cv::Point> hull;
        cv::convexHull(*std::max_element(cs.begin(), cs.end(), [](auto &a, auto &b) { return a.size() < b.size(); }), hull);
        fit = cv::fitEllipse(hull);
        fitMs = t.nsecsElapsed() / 1e6;
        fit = cv::fitEllipse(hull);
        }
        std::printf("  %s：霍夫找到 %zu 个圆", chip ? "有缺口" : "无缺口", circles.size());
        if (!circles.empty())
            std::printf("，最强的一个 (%.1f, %.1f) r=%.1f，圆心误差 %.2f，%.1f ms",
                        circles[0][0], circles[0][1], circles[0][2], cv::norm(cv::Point2d(circles[0][0], circles[0][1]) - truth), houghMs);
        std::printf("\n          凸包拟合 (%.1f, %.1f)，圆心误差 %.2f，%.1f ms\n", fit.center.x, fit.center.y,
                    cv::norm(cv::Point2d(fit.center) - truth), fitMs);
        if (chip) {
            cv::Mat shown;
            cv::cvtColor(img, shown, cv::COLOR_GRAY2BGR);
            for (const auto &c : circles)
                cv::circle(shown, cv::Point2f(c[0], c[1]), int(c[2]), cv::Scalar(60, 60, 240), 1, cv::LINE_AA);
            save("hough-circles", shown);
        }
    }

    std::printf("\n==== 2. 霍夫直线：三条已知的线 ====\n");
    // [region hough-lines]
    cv::Mat lines(200, 300, CV_8UC1, cv::Scalar(0));
    cv::line(lines, {20, 30}, {280, 30}, cv::Scalar(255), 1);      // 水平
    cv::line(lines, {150, 10}, {150, 190}, cv::Scalar(255), 1);    // 竖直
    cv::line(lines, {20, 180}, {200, 40}, cv::Scalar(255), 1);     // 斜线
    std::vector<cv::Vec2f> standard;                                // 每条：(ρ, θ)，无限长
    cv::HoughLines(lines, standard, 1, CV_PI / 180, 100);
    std::vector<cv::Vec4i> segments;                                // 每条：两个端点
    cv::HoughLinesP(lines, segments, 1, CV_PI / 180, 50, 30, 5);
    // [endregion]
    std::printf("  HoughLines（阈值 100 票）：%zu 条\n", standard.size());
    for (const auto &l : standard)
        std::printf("    ρ=%6.1f θ=%5.1f°\n", l[0], l[1] * 180 / CV_PI);
    std::printf("  HoughLinesP：%zu 段\n", segments.size());
    for (const auto &s : segments)
        std::printf("    (%d,%d) → (%d,%d)\n", s[0], s[1], s[2], s[3]);

    std::printf("\n==== 3. 仿射变换：旋转 30° 再转回来 ====\n");
    // [region affine]
    PartGenerator gen(20261003);
    cv::Mat part;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, part, cv::COLOR_BGR2GRAY);
    const cv::Point2f center(part.cols / 2.f, part.rows / 2.f);
    const cv::Mat M = cv::getRotationMatrix2D(center, 30, 1.0);    // 2×3 矩阵：绕中心逆时针 30°，不缩放
    const cv::Mat Minv = cv::getRotationMatrix2D(center, -30, 1.0);
    for (int interp : {cv::INTER_NEAREST, cv::INTER_LINEAR, cv::INTER_CUBIC}) {
        cv::Mat rotated, back;
        cv::warpAffine(part, rotated, M, part.size(), interp);
        cv::warpAffine(rotated, back, Minv, part.size(), interp);
        const cv::Rect inner(140, 100, 200, 160);                     // 只比较中间区域：四角转出去的部分丢了
        std::printf("  %-13s 转过去再转回来，中间区域平均误差 %.2f\n",
                    interp == cv::INTER_NEAREST ? "INTER_NEAREST" : interp == cv::INTER_LINEAR ? "INTER_LINEAR" : "INTER_CUBIC",
                    cv::norm(back(inner), part(inner), cv::NORM_L1) / inner.area());
        if (interp == cv::INTER_LINEAR) save("affine-rotated", rotated);
    }
    // 换一张没有噪声的图：一个边缘锐利的白方块，旋转后量边缘处有多「毛」
    cv::Mat square(200, 200, CV_8UC1, cv::Scalar(0));
    cv::rectangle(square, cv::Rect(50, 50, 100, 100), cv::Scalar(255), cv::FILLED);
    const cv::Mat R = cv::getRotationMatrix2D({100, 100}, 30, 1.0);
    for (int interp : {cv::INTER_NEAREST, cv::INTER_LINEAR}) {
        cv::Mat rot, ideal;
        cv::warpAffine(square, rot, R, square.size(), interp);
        // 理想结果：在 8 倍分辨率上旋转后缩小（相当于精确计算每个像素被方块覆盖了多少）
        cv::Mat big, bigRot;
        cv::resize(square, big, {}, 8, 8, cv::INTER_NEAREST);
        cv::warpAffine(big, bigRot, cv::getRotationMatrix2D({800, 800}, 30, 1.0), big.size(), cv::INTER_NEAREST);
        cv::resize(bigRot, ideal, square.size(), 0, 0, cv::INTER_AREA);
        std::printf("  无噪声的方块旋转 30°，%s 和理想结果的平均误差 %.2f\n",
                    interp == cv::INTER_NEAREST ? "INTER_NEAREST" : "INTER_LINEAR ", cv::norm(rot, ideal, cv::NORM_L1) / rot.total());
        save(interp == cv::INTER_NEAREST ? "affine-square-nearest" : "affine-square-linear", rot);
    }
    std::printf("  旋转矩阵：[%.3f %.3f %.1f; %.3f %.3f %.1f]\n", M.at<double>(0, 0), M.at<double>(0, 1), M.at<double>(0, 2),
                M.at<double>(1, 0), M.at<double>(1, 1), M.at<double>(1, 2));
    // [endregion]

    std::printf("\n==== 4. 透视变换：把斜着拍的矩形拉正 ====\n");
    // [region perspective]
    // 一块 200×120 的「标签」，四角坐标已知；模拟斜着拍，四个角变成任意四边形
    cv::Mat label(120, 200, CV_8UC1, cv::Scalar(230));
    cv::putText(label, "VC-0042", {18, 75}, cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(20), 3, cv::LINE_AA);
    const std::vector<cv::Point2f> flat = {{0, 0}, {200, 0}, {200, 120}, {0, 120}};
    const std::vector<cv::Point2f> tilted = {{60, 40}, {250, 70}, {230, 200}, {40, 160}};
    const cv::Mat H = cv::getPerspectiveTransform(flat, tilted);    // 3×3：4 对点正好解出 8 个未知数
    cv::Mat photo;
    cv::warpPerspective(label, photo, H, {300, 240});
    // 反过来：已知照片里四个角的位置，把它拉正
    const cv::Mat Hinv = cv::getPerspectiveTransform(tilted, flat);
    cv::Mat rectified;
    cv::warpPerspective(photo, rectified, Hinv, label.size());
    // [endregion]
    std::printf("  拉正后和原标签的平均误差 %.2f\n", cv::norm(rectified, label, cv::NORM_L1) / label.total());
    // [region not-affine]
    cv::Mat A = cv::getAffineTransform(std::vector<cv::Point2f>(flat.begin(), flat.begin() + 3).data(),
                                       std::vector<cv::Point2f>(tilted.begin(), tilted.begin() + 3).data());
    std::vector<cv::Point2f> fourth;
    cv::transform(std::vector<cv::Point2f>{flat[3]}, fourth, A);
    std::printf("  只用前三个角求仿射变换，第四个角被映射到 (%.0f, %.0f)，实际在 (%.0f, %.0f)\n", fourth[0].x, fourth[0].y, tilted[3].x, tilted[3].y);
    // [endregion]
    save("perspective-label", label);
    save("perspective-photo", photo);
    save("perspective-rectified", rectified);
    return 0;
}
