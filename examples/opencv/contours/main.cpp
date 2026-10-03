// 轮廓检索：findContours 的四种检索模式返回什么、层级数组怎么读
//
// 运行：./example_opencv_contours <输出目录>     输出见 output.txt

#include <QDir>
#include <QString>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char *argv[])
{
    const QString dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(dir);

    // [region shape]
    // 一个垫圈（外圆 + 内孔），内孔里掉进了一个小颗粒；旁边还有一个单独的小方块
    cv::Mat bin(240, 320, CV_8UC1, cv::Scalar(0));
    cv::circle(bin, cv::Point(130, 120), 90, cv::Scalar(255), cv::FILLED);   // 垫圈外圆
    cv::circle(bin, cv::Point(130, 120), 40, cv::Scalar(0), cv::FILLED);     // 内孔
    cv::circle(bin, cv::Point(130, 120), 10, cv::Scalar(255), cv::FILLED);   // 孔里的颗粒
    cv::rectangle(bin, cv::Rect(260, 30, 30, 30), cv::Scalar(255), cv::FILLED);
    // [endregion]

    const struct { int mode; const char *name; } modes[] = {
        {cv::RETR_EXTERNAL, "RETR_EXTERNAL"}, {cv::RETR_LIST, "RETR_LIST"},
        {cv::RETR_CCOMP, "RETR_CCOMP"}, {cv::RETR_TREE, "RETR_TREE"},
    };
    for (const auto &m : modes) {
        // [region find]
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;   // 每个轮廓 4 个数：[下一个同级, 上一个同级, 第一个子轮廓, 父轮廓]，没有为 -1
        cv::findContours(bin, contours, hierarchy, m.mode, cv::CHAIN_APPROX_NONE);
        // [endregion]
        std::printf("==== %s：%zu 个轮廓 ====\n", m.name, contours.size());
        std::printf("  编号  面积      [下一个 上一个 子 父]\n");
        for (size_t i = 0; i < contours.size(); ++i) {
            const cv::Vec4i &h = hierarchy[i];
            std::printf("  %2zu  %8.0f   [%3d %4d %4d %3d]\n", i, cv::contourArea(contours[i]), h[0], h[1], h[2], h[3]);
        }
    }

    // [region approx]
    std::printf("\n==== 轮廓点的存法 ====\n");
    std::vector<std::vector<cv::Point>> none, simple;
    cv::findContours(bin, none, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    cv::findContours(bin, simple, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    for (size_t i = 0; i < none.size(); ++i)
        std::printf("  外轮廓 %zu：CHAIN_APPROX_NONE %zu 个点，CHAIN_APPROX_SIMPLE %zu 个点，面积 %.0f / %.0f\n", i,
                    none[i].size(), simple[i].size(), cv::contourArea(none[i]), cv::contourArea(simple[i]));
    // [endregion]

    // 配图：RETR_CCOMP 的结果，最外层（父 = -1）画绿色，第二层（洞）画红色
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(bin, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_NONE);
    cv::Mat shown;
    cv::cvtColor(bin / 3, shown, cv::COLOR_GRAY2BGR);
    for (size_t i = 0; i < contours.size(); ++i) {
        const bool top = hierarchy[i][3] == -1;
        cv::drawContours(shown, contours, int(i), top ? cv::Scalar(90, 210, 90) : cv::Scalar(60, 60, 240), 2);
        const cv::Rect b = cv::boundingRect(contours[i]);
        cv::putText(shown, std::to_string(i), cv::Point(b.x + 2, b.y + 14), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }
    cv::imwrite((dir + QStringLiteral("/contours-input.png")).toStdString(), bin);
    cv::imwrite((dir + QStringLiteral("/contours-ccomp.png")).toStdString(), shown);
    return 0;
}
