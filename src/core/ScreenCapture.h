#pragma once

#include <QImage>
#include <opencv2/core.hpp>

/**
 * @brief 跨平台屏幕捕获与 Qt/OpenCV 图像转换工具
 * 纯 Qt 跨平台实现，无需调用平台特定的 Win32 API。
 */
class ScreenCapture {
public:
    // 捕获指定屏幕（默认主屏，跨平台支持 Windows/macOS/Linux）
    static QImage grabScreen(int screenIndex = 0);

    // 捕获指定屏幕的局部矩形区域 (用于动态 ROI / 显微跟随)
    static QImage grabScreenRegion(int x, int y, int w, int h, int screenIndex = 0);

    // QImage 转换为 OpenCV cv::Mat (BGR 格式)
    static cv::Mat qImageToMat(const QImage &image);

    // OpenCV cv::Mat 转换为 QImage (RGB32 / Grayscale 格式)
    static QImage matToQImage(const cv::Mat &mat);
};
