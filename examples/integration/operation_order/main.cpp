#include "ImageOperations.h"
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <stdexcept>
static cv::Mat apply(const cv::Mat &image,ImageOperations::Operation operation,const ImageOperations::Params &params) {
    const auto result=ImageOperations::run(image,operation,params);
    if(!result.error.isEmpty())throw std::runtime_error(result.error.toStdString());
    return result.image;
}
int main() {
    try {
        ImageOperations::Params params;params.kernelSize=3;params.threshold=160;
        // [region threshold-order]
        cv::Mat pulse=(cv::Mat_<uchar>(1,7)<<0,0,0,255,0,0,0);
        const auto blurred=apply(pulse,ImageOperations::Operation::GaussianBlur,params);
        const auto blurThenThreshold=apply(blurred,ImageOperations::Operation::BinaryThreshold,params);
        const auto binary=apply(pulse,ImageOperations::Operation::BinaryThreshold,params);
        const auto thresholdThenBlur=apply(binary,ImageOperations::Operation::GaussianBlur,params);
        std::cout << "input=" << pulse << "\nblur intermediate=" << blurred
                  << "\nblur -> threshold=" << blurThenThreshold
                  << "\nthreshold intermediate=" << binary
                  << "\nthreshold -> blur=" << thresholdThenBlur << '\n';
        // [endregion]
        // [region mask-order]
        cv::Mat step=(cv::Mat_<uchar>(1,7)<<0,0,0,200,200,200,200);
        cv::Mat mask=(cv::Mat_<uchar>(1,7)<<255,255,255,255,0,0,0),masked,blurThenMask;
        cv::bitwise_and(step,mask,masked);
        const auto maskThenBlur=apply(masked,ImageOperations::Operation::GaussianBlur,params);
        cv::bitwise_and(apply(step,ImageOperations::Operation::GaussianBlur,params),mask,blurThenMask);
        std::cout << "mask -> blur=" << maskThenBlur
                  << "\nblur -> mask=" << blurThenMask << '\n';
        // [endregion]
        // [region canny-order]
        cv::Mat scene(48,64,CV_8UC1);cv::RNG rng(5);
        for(int y=0;y<scene.rows;++y)for(int x=0;x<scene.cols;++x)
            scene.at<uchar>(y,x)=cv::saturate_cast<uchar>((x<32?45:195)+rng.uniform(-30,31));
        params.kernelSize=5;params.lowThreshold=50;params.highThreshold=100;
        const auto smooth=apply(scene,ImageOperations::Operation::GaussianBlur,params);
        const auto smoothEdges=apply(smooth,ImageOperations::Operation::Canny,params);
        const auto rawEdges=apply(scene,ImageOperations::Operation::Canny,params);
        const auto blurredEdges=apply(rawEdges,ImageOperations::Operation::GaussianBlur,params);
        std::cout << "raw Canny nonzero=" << cv::countNonZero(rawEdges)
                  << "\nblur -> Canny nonzero=" << cv::countNonZero(smoothEdges)
                  << " intermediateGray=" << cv::countNonZero((smoothEdges!=0)&(smoothEdges!=255))
                  << "\nCanny -> blur nonzero=" << cv::countNonZero(blurredEdges)
                  << " intermediateGray=" << cv::countNonZero((blurredEdges!=0)&(blurredEdges!=255)) << '\n';
        // [endregion]
    } catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
