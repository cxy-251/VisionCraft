#include "ScreenCapture.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <opencv2/imgproc.hpp>

QImage ScreenCapture::grabScreen(int screenIndex) {
    auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return QImage();
    }
    if (screenIndex < 0 || screenIndex >= screens.size()) {
        screenIndex = 0;
    }
    QScreen *screen = screens[screenIndex];
    if (!screen) return QImage();

    // 跨平台通过 QScreen 抓取整屏像素
    QPixmap pixmap = screen->grabWindow(0);
    return pixmap.toImage();
}

QImage ScreenCapture::grabScreenRegion(int x, int y, int w, int h, int screenIndex) {
    auto screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return QImage();
    }
    if (screenIndex < 0 || screenIndex >= screens.size()) {
        screenIndex = 0;
    }
    QScreen *screen = screens[screenIndex];
    if (!screen) return QImage();

    QPixmap pixmap = screen->grabWindow(0, x, y, w, h);
    return pixmap.toImage();
}

cv::Mat ScreenCapture::qImageToMat(const QImage &image) {
    if (image.isNull()) return cv::Mat();

    // 零额外拷贝极速路径：Windows 平台 grabWindow 生成的 32 位格式底层即为 BGRA
    if (image.format() == QImage::Format_RGB32 || 
        image.format() == QImage::Format_ARGB32 || 
        image.format() == QImage::Format_ARGB32_Premultiplied) {
        cv::Mat bgraMat(image.height(), image.width(), CV_8UC4,
                        const_cast<uchar*>(image.bits()),
                        static_cast<size_t>(image.bytesPerLine()));
        cv::Mat bgrMat;
        cv::cvtColor(bgraMat, bgrMat, cv::COLOR_BGRA2BGR);
        return bgrMat;
    }

    QImage conv = image.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(conv.height(), conv.width(), CV_8UC3,
                const_cast<uchar*>(conv.bits()),
                static_cast<size_t>(conv.bytesPerLine()));
    
    cv::Mat bgrMat;
    cv::cvtColor(mat, bgrMat, cv::COLOR_RGB2BGR);
    return bgrMat.clone(); // 保证数据深拷贝脱离局部 QImage
}

QImage ScreenCapture::matToQImage(const cv::Mat &mat) {
    if (mat.empty()) return QImage();

    if (mat.type() == CV_8UC1) {
        // 单通道灰度图
        QImage image(mat.data, mat.cols, mat.rows, 
                     static_cast<int>(mat.step), QImage::Format_Grayscale8);
        return image.copy();
    } 
    else if (mat.type() == CV_8UC3) {
        // 3通道 BGR 图
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGR2RGB);
        QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows, 
                     static_cast<int>(rgbMat.step), QImage::Format_RGB888);
        return image.copy();
    } 
    else if (mat.type() == CV_8UC4) {
        // 4通道 BGRA 图
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, cv::COLOR_BGRA2RGBA);
        QImage image(rgbMat.data, rgbMat.cols, rgbMat.rows, 
                     static_cast<int>(rgbMat.step), QImage::Format_RGBA8888);
        return image.copy();
    }
    
    return QImage();
}
