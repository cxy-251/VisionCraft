#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
int main() {
    // [region shape]
    cv::Mat image(2,4,CV_8UC3);
    for(int y=0;y<image.rows;++y)
        for(int x=0;x<image.cols;++x)
            image.at<cv::Vec3b>(y,x)=cv::Vec3b(10*y+x,20+y,30+x);
    std::cout << "rows=" << image.rows << " cols=" << image.cols
              << " channels=" << image.channels() << " depth=" << image.depth()
              << " type=" << image.type() << " elemSize=" << image.elemSize()
              << " elemSize1=" << image.elemSize1() << " step=" << image.step[0] << '\n';
    std::cout << "BGR(y=1,x=2)=" << image.at<cv::Vec3b>(1,2) << '\n';
    // [endregion]
    // [region roi]
    cv::Mat roi=image(cv::Rect(1,0,2,2));
    cv::Mat packed=roi.clone();
    std::cout << "ROI: cols=" << roi.cols << " rowBytes=" << roi.cols*roi.elemSize()
              << " step=" << roi.step[0] << " continuous=" << roi.isContinuous() << '\n';
    std::cout << "clone: step=" << packed.step[0] << " continuous=" << packed.isContinuous() << '\n';
    // 按各自的行步长定位。不能把这个ROI当成紧密排列的2×2图像线性遍历。
    for(int y=0;y<roi.rows;++y) {
        const auto *row=roi.ptr<cv::Vec3b>(y);
        std::cout << "ROI row " << y << ": " << row[0] << ' ' << row[1] << '\n';
    }
    roi.at<cv::Vec3b>(0,0)[0]=99;
    std::cout << "ROI write: parent=" << image.at<cv::Vec3b>(0,1)
              << " clone=" << packed.at<cv::Vec3b>(0,0) << '\n';
    // [endregion]
    // [region color]
    cv::Mat rgb;
    cv::cvtColor(image,rgb,cv::COLOR_BGR2RGB);
    std::cout << "RGB(y=1,x=2)=" << rgb.at<cv::Vec3b>(1,2) << '\n';
    // [endregion]
    // [region depth]
    cv::Mat values=(cv::Mat_<uchar>(1,3)<<10,120,250),plus,minus,normalized;
    cv::add(values,cv::Scalar::all(20),plus);
    cv::subtract(values,cv::Scalar::all(20),minus);
    values.convertTo(normalized,CV_32F,1.0/255.0);
    std::cout << "+20 saturated=" << plus << "\n-20 saturated=" << minus
              << "\nnormalized=" << normalized << '\n';
    const uchar wrapped=static_cast<uchar>(250+20);
    std::cout << "unsigned byte cast(270)=" << int(wrapped) << '\n';
    // [endregion]
}
