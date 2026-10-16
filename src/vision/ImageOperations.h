#pragma once
#include <QString>
#include <opencv2/core.hpp>

// 从旧 ImageProcessorPage 提取的图像运算；Widgets 与后续 QML 共用。
// 接收8位灰度/BGR/BGRA图，不修改输入，不访问界面、文件或设备。
namespace ImageOperations {
enum class Operation { Grayscale, GaussianBlur, BinaryThreshold, Canny };
struct Params {
    int kernelSize=3;
    double threshold=128;
    double lowThreshold=50;
    double highThreshold=150;
};
struct Result {
    cv::Mat image;
    QString error;
};
Result run(const cv::Mat &input, Operation operation, const Params &params={});
}
