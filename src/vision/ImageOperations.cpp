#include "ImageOperations.h"
#include <opencv2/imgproc.hpp>
#include <cmath>

namespace ImageOperations {
// [region operations]
Result run(const cv::Mat &input, Operation operation, const Params &params)
{
    if(input.empty() || input.depth()!=CV_8U ||
        (input.channels()!=1 && input.channels()!=3 && input.channels()!=4))
        return {{},QStringLiteral("需要8位灰度、BGR或BGRA图像")};
    if(operation==Operation::GaussianBlur &&
        (params.kernelSize<1 || params.kernelSize>255 || params.kernelSize%2==0))
        return {{},QStringLiteral("高斯核大小必须为1–255之间的奇数")};
    auto thresholdValid=[](double value) { return std::isfinite(value) && value>=0 && value<=255; };
    if((operation==Operation::BinaryThreshold && !thresholdValid(params.threshold)) ||
        (operation==Operation::Canny && (!thresholdValid(params.lowThreshold) || !thresholdValid(params.highThreshold))))
        return {{},QStringLiteral("阈值必须在0–255之间")};
    try {
        cv::Mat result;
        if(operation==Operation::GaussianBlur) {
            cv::GaussianBlur(input,result,cv::Size(params.kernelSize,params.kernelSize),0);
            return {result,{}};
        }
        cv::Mat gray;
        if(input.channels()==1) gray=input;
        else cv::cvtColor(input,gray,input.channels()==4 ? cv::COLOR_BGRA2GRAY : cv::COLOR_BGR2GRAY);
        switch(operation) {
        case Operation::Grayscale: result=gray.clone(); break;
        case Operation::BinaryThreshold: cv::threshold(gray,result,params.threshold,255,cv::THRESH_BINARY); break;
        case Operation::Canny: cv::Canny(gray,result,params.lowThreshold,params.highThreshold); break;
        default: return {{},QStringLiteral("不支持的图像运算")};
        }
        return {result,{}};
    } catch(const cv::Exception &e) {
        return {{},QString::fromUtf8(e.what())};
    }
}
// [endregion]
}
