// 边缘：Sobel 求亮度变化（梯度），Canny 把梯度变成一像素宽的边缘线
//
// 运行：./example_opencv_edges <输出目录>     输出见 output.txt

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
    auto save = [&](const char *n, const cv::Mat &m) { cv::imwrite((dir + "/" + n + ".png").toStdString(), m); };

    PartGenerator gen(20261003);
    cv::Mat gray;
    cv::cvtColor(gen.make(VC_DEFECT_NONE, 0.0).image, gray, cv::COLOR_BGR2GRAY);

    std::printf("==== 1. Sobel：输出类型要能装下负数 ====\n");
    // [region sobel]
    cv::Mat gx, gy, gx8;
    cv::Sobel(gray, gx, CV_16S, 1, 0, 3);   // x 方向导数：左暗右亮为正，左亮右暗为负
    cv::Sobel(gray, gy, CV_16S, 0, 1, 3);   // y 方向导数
    cv::Sobel(gray, gx8, CV_8U, 1, 0, 3);   // 错误示范：8 位无符号装不下负数
    double mn, mx, mn8, mx8;
    cv::minMaxLoc(gx, &mn, &mx);
    cv::minMaxLoc(gx8, &mn8, &mx8);
    std::printf("  CV_16S：x 方向导数范围 %.0f ~ %.0f\n", mn, mx);
    std::printf("  CV_8U ：范围 %.0f ~ %.0f（负的全被截成 0）\n", mn8, mx8);
    cv::Mat mag;
    cv::Mat fx, fy;
    gx.convertTo(fx, CV_32F);
    gy.convertTo(fy, CV_32F);
    cv::magnitude(fx, fy, mag);             // 梯度大小 = √(gx² + gy²)，和方向无关
    // [endregion]

    // 配图：把带符号的导数映射到 0~255，0 对应灰色 128
    cv::Mat shownX, shownMag, shown8;
    gx.convertTo(shownX, CV_8U, 0.125, 128);
    mag.convertTo(shownMag, CV_8U, 0.25);
    save("edges-sobel-x", shownX);
    save("edges-sobel-x-8u", gx8);
    save("edges-magnitude", shownMag);

    std::printf("\n==== 2. Canny 的两个阈值 ====\n");
    // [region canny]
    // 换一件有划痕的工件，不模糊：传送带纹理、噪声、划痕、垫圈边缘的梯度强弱不同
    cv::Mat scratched;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, scratched, cv::COLOR_BGR2GRAY);
    for (auto [lo, hi] : {std::pair{20, 60}, std::pair{60, 180}, std::pair{150, 450}, std::pair{400, 1200}}) {
        cv::Mat edges, labels;
        cv::Canny(scratched, edges, lo, hi);  // 梯度 > hi 一定是边；< lo 一定不是；中间的只有连着「一定是边」的才保留
        const int pieces = cv::connectedComponents(edges, labels, 8) - 1;
        std::printf("  Canny(%3d, %3d)：边缘像素 %5d，连成 %3d 段\n", lo, hi, cv::countNonZero(edges), pieces);
        save(QStringLiteral("edges-canny-%1").arg(lo).toUtf8().constData(), edges);
    }
    // [endregion]
    return 0;
}
