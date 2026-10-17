#include <opencv2/imgproc.hpp>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

// [region clustering]
static void clustering() {
    // 每行一个BGR样本，CV_32F；不是把8位图的每一行误当成一个样本。
    cv::Mat points=(cv::Mat_<float>(8,3) <<
        10,10,10, 12,10,10, 8,10,10, 10,12,10,
        230,220,210, 232,220,210, 228,220,210, 230,222,210);
    for(int k: {1,2}) {
        cv::Mat labels(8,1,CV_32S),centers;
        for(int i=0;i<8;++i) labels.at<int>(i)=k==1?0:i/4;
        const double sse=cv::kmeans(points,k,labels,
            cv::TermCriteria(cv::TermCriteria::COUNT|cv::TermCriteria::EPS,30,0.0001),
            1,cv::KMEANS_USE_INITIAL_LABELS,centers);
        std::cout << "K=" << k << " SSE=" << sse << '\n';
        for(int cluster=0;cluster<k;++cluster) {
            int count=0; for(int i=0;i<labels.rows;++i) count+=labels.at<int>(i)==cluster;
            std::cout << "  center=" << centers.row(cluster) << " count=" << count << '\n';
        }
    }
}
// [endregion]
// [region overlap]
static void overlap(const char *name,const cv::Mat &prediction,const cv::Mat &truth) {
    cv::Mat p,t,hit,joined,different;
    cv::compare(prediction,0,p,cv::CMP_NE); cv::compare(truth,0,t,cv::CMP_NE);
    cv::bitwise_and(p,t,hit); cv::bitwise_or(p,t,joined); cv::bitwise_xor(p,t,different);
    const int tp=cv::countNonZero(hit), np=cv::countNonZero(p), nt=cv::countNonZero(t);
    const int unionSize=cv::countNonZero(joined);
    std::cout << name << " TP=" << tp << " FP=" << np-tp << " FN=" << nt-tp
              << " pixelAccuracy=" << 1.0-double(cv::countNonZero(different))/truth.total();
    if(unionSize==0) std::cout << " IoU=N/A Dice=N/A (both empty)\n";
    else std::cout << " IoU=" << double(tp)/unionSize << " Dice=" << 2.0*tp/(np+nt) << '\n';
}
// [endregion]
// [region error]
static void error(const char *name,const cv::Mat &image,const cv::Mat &reference) {
    // 此例固定为同尺寸、单通道8位图；先按浮点作差，避免无符号饱和截断。
    cv::Mat a,b,diff; image.convertTo(a,CV_64F); reference.convertTo(b,CV_64F);
    diff=a-b;
    const double mse=cv::sum(diff.mul(diff))[0]/reference.total();
    const double psnr=mse==0 ? std::numeric_limits<double>::infinity() : 10*std::log10(255.0*255.0/mse);
    std::cout << name << " MSE=" << mse << " PSNR=" << psnr << " dB\n";
}
// [endregion]
int main() {
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== color samples and cluster compactness ===\n"; clustering();
    // [region masks]
    std::cout << "=== foreground overlap, 10000 pixels ===\n";
    cv::Mat truth=cv::Mat::zeros(100,100,CV_8UC1), empty=truth.clone(), shifted=truth.clone();
    cv::rectangle(truth,{20,20,10,10},255,cv::FILLED);
    cv::rectangle(shifted,{21,20,10,10},1,cv::FILLED); // 1和255在这里都表示前景。
    overlap("empty prediction",empty,truth);
    overlap("shifted one pixel",shifted,truth);
    overlap("perfect",truth,truth);
    overlap("both empty",empty,empty);
    // [endregion]
    // [region pairs]
    std::cout << "=== equal MSE, different spatial errors ===\n";
    cv::Mat reference(8,8,CV_8UC1);
    for(int y=0;y<8;++y)for(int x=0;x<8;++x) reference.at<uchar>(y,x)=uchar(80+20*((x+y)%4));
    cv::Mat offset,local=reference.clone(); cv::add(reference,cv::Scalar(5),offset);
    local.at<uchar>(0,0)+=40;
    error("identical",reference,reference);
    error("all 64 pixels +5",offset,reference);
    error("one pixel +40",local,reference);
    // [endregion]
}
