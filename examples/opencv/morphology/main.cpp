// 形态学：腐蚀、膨胀、开运算、闭运算，以及工位检测用到的黑帽变换
//
// 运行：./example_opencv_morphology <输出目录>     打印的数字见 output.txt，图存到输出目录

#include <QDir>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

static QString g_dir;
static void save(const char *name, const cv::Mat &m) { cv::imwrite((g_dir + "/" + name + ".png").toStdString(), m); }

// 统计白色像素数和连通块数
static void report(const char *what, const cv::Mat &bin)
{
    cv::Mat labels;
    const int blobs = cv::connectedComponents(bin, labels, 8) - 1;   // 减去背景
    std::printf("  %-10s 白色像素 %5d，连通块 %d\n", what, cv::countNonZero(bin), blobs);
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);

    // [region shapes]
    // 一张 200×120 的二值图：一个大矩形（中间有一个小洞、右边有一条 2 像素的细缝），外加几个孤立噪点
    cv::Mat bin(120, 200, CV_8UC1, cv::Scalar(0));
    cv::rectangle(bin, cv::Rect(30, 25, 140, 70), cv::Scalar(255), cv::FILLED);
    cv::circle(bin, cv::Point(80, 60), 3, cv::Scalar(0), cv::FILLED);           // 小洞
    cv::rectangle(bin, cv::Rect(120, 25, 2, 70), cv::Scalar(0), cv::FILLED);     // 细缝：把矩形切成两块
    for (const cv::Point p : {cv::Point(10, 10), cv::Point(185, 15), cv::Point(15, 105), cv::Point(190, 100)})
        cv::circle(bin, p, 1, cv::Scalar(255), cv::FILLED);                      // 噪点
    // [endregion]

    // [region ops]
    const cv::Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));   // 5×5 的圆形结构元素
    cv::Mat eroded, dilated, opened, closed;
    cv::erode(bin, eroded, k);                                 // 腐蚀：结构元素整个落在白色里，中心才保留
    cv::dilate(bin, dilated, k);                               // 膨胀：结构元素碰到白色，中心就变白
    cv::morphologyEx(bin, opened, cv::MORPH_OPEN, k);          // 开运算 = 先腐蚀再膨胀
    cv::morphologyEx(bin, closed, cv::MORPH_CLOSE, k);         // 闭运算 = 先膨胀再腐蚀
    // [endregion]

    std::printf("==== 1. 四种基本运算（5×5 圆形结构元素） ====\n");
    report("原图", bin);
    report("腐蚀", eroded);
    report("膨胀", dilated);
    report("开运算", opened);
    report("闭运算", closed);
    // [region size]
    std::printf("  小洞（直径 7 像素）的中心 (80,60)：5×5 闭运算后 %s\n", closed.at<uchar>(60, 80) ? "被填上" : "还在");
    cv::Mat closed9;
    cv::morphologyEx(bin, closed9, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9)));
    std::printf("  换成 9×9 的结构元素再做闭运算：%s\n", closed9.at<uchar>(60, 80) ? "被填上" : "还在");
    // [endregion]
    save("morph-input", bin);
    save("morph-erode", eroded);
    save("morph-dilate", dilated);
    save("morph-open", opened);
    save("morph-close", closed);

    std::printf("\n==== 2. 光照不均时找暗斑：直接阈值 vs 黑帽 ====\n");
    // [region lighting]
    // 一块亮面，左亮右暗（亮度从 220 渐变到 120），上面有一个比周围暗 40 的小斑点
    cv::Mat surface(120, 300, CV_8UC1);
    for (int x = 0; x < surface.cols; ++x)
        surface.col(x).setTo(cv::Scalar(220 - 100 * x / (surface.cols - 1)));
    cv::Mat spotMask(surface.size(), CV_8UC1, cv::Scalar(0));
    cv::circle(spotMask, cv::Point(60, 60), 5, cv::Scalar(255), cv::FILLED);
    cv::subtract(surface, cv::Scalar(40), surface, spotMask);              // 斑点处暗 40
    // [endregion]

    // [region compare]
    cv::Mat direct;
    cv::threshold(surface, direct, 170, 255, cv::THRESH_BINARY_INV);       // 直接阈值：比 170 暗的算异常
    cv::Mat blackhat, found;
    cv::morphologyEx(surface, blackhat, cv::MORPH_BLACKHAT,
                     cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21)));   // 黑帽 = 闭运算 − 原图
    cv::threshold(blackhat, found, 18, 255, cv::THRESH_BINARY);            // 和工位一样的阈值 18
    // [endregion]
    const int spotPixels = cv::countNonZero(spotMask);
    auto score = [&](const char *what, const cv::Mat &m) {
        cv::Mat hit, falseAlarm;
        cv::bitwise_and(m, spotMask, hit);
        cv::bitwise_and(m, ~spotMask, falseAlarm);
        std::printf("  %-22s 斑点 %d 个像素里找到 %d 个，斑点以外误报 %d 个像素\n", what, spotPixels,
                    cv::countNonZero(hit), cv::countNonZero(falseAlarm));
    };
    score("直接阈值（< 170）", direct);
    score("黑帽 21×21 + 阈值 18", found);
    double bhMax, bhOutside;
    cv::minMaxLoc(blackhat, nullptr, &bhMax);
    cv::Mat far;   // 离斑点远一些的区域（斑点外扩 15 像素以外）
    cv::dilate(spotMask, far, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(31, 31)));
    cv::minMaxLoc(blackhat, nullptr, &bhOutside, nullptr, nullptr, ~far);
    std::printf("  黑帽结果：斑点处最大 %.0f，远离斑点的地方最大 %.0f\n", bhMax, bhOutside);
    save("lighting-input", surface);
    save("lighting-direct", direct);
    cv::Mat shown;
    blackhat.convertTo(shown, -1, 4.0);
    save("lighting-blackhat-x4", shown);
    save("lighting-found", found);
    return 0;
}
