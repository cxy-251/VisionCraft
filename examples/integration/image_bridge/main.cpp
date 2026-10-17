#include "core/ScreenCapture.h"
#include <QCoreApplication>
#include <opencv2/core.hpp>
#include <iostream>
static void pixel(const char *name,const QImage &image,int x,int y) {
    const auto color=image.pixelColor(x,y);
    std::cout << name << " RGB=" << color.red() << ',' << color.green() << ',' << color.blue() << '\n';
}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    // [region bridge]
    QImage delivered;
    {
        cv::Mat parent(2,4,CV_8UC3,cv::Scalar(10,20,200));
        cv::Mat roi=parent(cv::Rect(1,0,2,2));
        std::cout << "Mat ROI: width=" << roi.cols << " height=" << roi.rows
                  << " channels=" << roi.channels() << " step=" << roi.step[0] << '\n';
        // 使用主应用已有转换。转换后的QImage自己持有像素，不借用局部Mat。
        delivered=ScreenCapture::matToQImage(roi);
        std::cout << "QImage: width=" << delivered.width() << " height=" << delivered.height()
                  << " bytesPerLine=" << delivered.bytesPerLine() << '\n';
        pixel("delivered",delivered,0,0);
        parent.setTo(cv::Scalar(0,255,0));
    } // parent和roi在这里释放，delivered仍可读取。
    pixel("after Mat scope",delivered,1,1);
    cv::Mat returned=ScreenCapture::qImageToMat(delivered);
    std::cout << "returned BGR=" << returned.at<cv::Vec3b>(1,1) << '\n';
    // [endregion]
    // [region ownership]
    cv::Mat rgb(1,2,CV_8UC3,cv::Scalar(255,0,0));
    QImage borrowed(static_cast<const uchar*>(rgb.data),rgb.cols,rgb.rows,
                    qsizetype(rgb.step[0]),QImage::Format_RGB888);
    QImage owned=borrowed.copy();
    rgb.setTo(cv::Scalar(0,255,0)); // 所有者仍活着：安全地观察借用与复制的差别。
    pixel("borrowed after owner write",borrowed,0,0);
    pixel("owned after owner write",owned,0,0);
    // 不在rgb释放后继续读borrowed；那是悬空内存，不作为可运行演示。
    // [endregion]
}
