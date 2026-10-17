#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
int main() {
    // [region correlation]
    cv::Mat image=(cv::Mat_<float>(3,3)<<1,2,3,4,5,6,7,8,9);
    cv::Mat kernel=(cv::Mat_<float>(3,3)<<0,0,0,-1,0,1,0,0,0);
    cv::Mat correlated,flippedKernel,convolved;
    cv::filter2D(image,correlated,CV_32F,kernel,{-1,-1},0,cv::BORDER_CONSTANT);
    cv::flip(kernel,flippedKernel,-1); // 水平、垂直各翻转一次，等于旋转180度。
    cv::filter2D(image,convolved,CV_32F,flippedKernel,{-1,-1},0,cv::BORDER_CONSTANT);
    std::cout << "input=" << image << "\nkernel=" << kernel
              << "\ncorrelation=" << correlated << '\n';
    std::cout << "center: correlation=" << correlated.at<float>(1,1)
              << " convolution=" << convolved.at<float>(1,1) << '\n';
    // [endregion]
    // [region border]
    for(const auto &entry:std::vector<std::pair<const char*,int>>{
            {"constant",cv::BORDER_CONSTANT},{"replicate",cv::BORDER_REPLICATE},{"reflect101",cv::BORDER_REFLECT_101}}) {
        cv::Mat result;
        cv::filter2D(image,result,CV_32F,kernel,{-1,-1},0,entry.second);
        std::cout << entry.first << ": corner=" << result.at<float>(0,0)
                  << " center=" << result.at<float>(1,1) << '\n';
    }
    cv::Mat shifted;
    cv::filter2D(image,shifted,CV_32F,kernel,{0,1},0,cv::BORDER_CONSTANT);
    std::cout << "anchor(0,1): center=" << shifted.at<float>(1,1) << '\n';
    // [endregion]
    // [region mean]
    cv::Mat meanKernel=cv::Mat::ones(3,3,CV_32F)/9.0,mean;
    cv::filter2D(image,mean,CV_32F,meanKernel,{-1,-1},0,cv::BORDER_CONSTANT);
    std::cout << "mean: center=" << mean.at<float>(1,1)
              << " corner=" << mean.at<float>(0,0) << '\n';
    // [endregion]
    // [region depth]
    cv::Mat falling=(cv::Mat_<uchar>(1,3)<<200,100,10);
    cv::Mat difference=(cv::Mat_<float>(1,3)<<-1,0,1),signedResult,byteResult,display;
    cv::filter2D(falling,signedResult,CV_16S,difference,{-1,-1},0,cv::BORDER_CONSTANT);
    cv::filter2D(falling,byteResult,CV_8U,difference,{-1,-1},0,cv::BORDER_CONSTANT);
    cv::convertScaleAbs(signedResult,display);
    std::cout << "falling center: signed=" << signedResult.at<short>(0,1)
              << " byte=" << int(byteResult.at<uchar>(0,1))
              << " abs-display=" << int(display.at<uchar>(0,1)) << '\n';
    // [endregion]
}
