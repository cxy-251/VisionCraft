// 位运算与掩膜：与、或、异或、非，以及「只在掩膜里算」
//
// 运行：./example_opencv_masks <输出目录>     输出见 output.txt

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

    // [region masks]
    // 两个圆形区域：A 在左，B 在右，部分重叠。掩膜约定：255 = 在区域里，0 = 不在
    cv::Mat a(120, 200, CV_8UC1, cv::Scalar(0)), b = a.clone();
    cv::circle(a, cv::Point(80, 60), 50, cv::Scalar(255), cv::FILLED);
    cv::circle(b, cv::Point(120, 60), 50, cv::Scalar(255), cv::FILLED);
    cv::Mat andM, orM, xorM, notA;
    cv::bitwise_and(a, b, andM);   // 两个都在：交集
    cv::bitwise_or(a, b, orM);     // 任一在：并集
    cv::bitwise_xor(a, b, xorM);   // 只在其中一个：对称差
    cv::bitwise_not(a, notA);      // 不在 A 里
    // [endregion]
    std::printf("==== 1. 像素个数 ====\n");
    std::printf("  A %d  B %d  A与B %d  A或B %d  A异或B %d  非A %d（图共 %d）\n", cv::countNonZero(a), cv::countNonZero(b),
                cv::countNonZero(andM), cv::countNonZero(orM), cv::countNonZero(xorM), cv::countNonZero(notA), int(a.total()));
    save("mask-a", a); save("mask-b", b); save("mask-and", andM); save("mask-or", orM); save("mask-xor", xorM);

    std::printf("\n==== 2. 只在掩膜里算 ====\n");
    // [region use]
    cv::Mat img(120, 200, CV_8UC1);
    for (int x = 0; x < img.cols; ++x)
        img.col(x).setTo(x);                                  // 从左到右亮度 0~199
    std::printf("  整张图的平均亮度 %.1f，只看 A 里的 %.1f，只看 B 里的 %.1f\n",
                cv::mean(img)[0], cv::mean(img, a)[0], cv::mean(img, b)[0]);
    cv::Mat onlyA(img.size(), img.type(), cv::Scalar(0));
    img.copyTo(onlyA, a);                                     // 只把 A 里的像素拷过去，其余保持 0
    std::printf("  copyTo(onlyA, A) 之后，onlyA 里非零像素 %d 个\n", cv::countNonZero(onlyA));
    // [endregion]

    std::printf("\n==== 3. 掩膜的值不是 255 会怎样 ====\n");
    // [region pitfall]
    cv::Mat a01 = a / 255;                                    // 同一个区域，但用 0/1 表示
    cv::Mat r1 = img & a;                                     // 和 255 按位与：值不变
    cv::Mat r2 = img & a01;                                   // 和 1 按位与：只剩最低一位（奇数变 1，偶数变 0）
    double max1, max2;
    cv::minMaxLoc(r1, nullptr, &max1);
    cv::minMaxLoc(r2, nullptr, &max2);
    std::printf("  img & A(0/255)：最大值 %.0f；img & A(0/1)：最大值 %.0f\n", max1, max2);
    std::printf("  cv::mean(img, A(0/1)) = %.1f（函数的掩膜参数只看「非零」，0/1 也行）\n", cv::mean(img, a01)[0]);
    // [endregion]
    return 0;
}
