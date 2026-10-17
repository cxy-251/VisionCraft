#include <opencv2/imgproc.hpp>
#include <iostream>
// [region describe]
static void describe(const char *name,const cv::Mat &mask,const cv::Mat &truth) {
    cv::Mat hit,labels;cv::bitwise_and(mask,truth,hit);
    const int inside=cv::countNonZero(hit),total=cv::countNonZero(mask);
    const int objects=cv::connectedComponents(mask,labels,8)-1;
    std::vector<std::vector<cv::Point>> contours;std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_SIMPLE);
    int children=0;for(const auto &h:hierarchy)children+=h[3]>=0;
    std::cout << name << ": selected=" << total << " inside=" << inside
              << " outside=" << total-inside << " missed=" << cv::countNonZero(truth)-inside
              << " objects=" << objects << " childContours=" << children << '\n';
}
// [endregion]
int main() {
    // [region scene]
    cv::Mat background(96,128,CV_8UC1),truth=cv::Mat::zeros(background.size(),CV_8UC1);
    for(int y=0;y<background.rows;++y)for(int x=0;x<background.cols;++x)background.at<uchar>(y,x)=uchar(30+x);
    cv::rectangle(truth,{14,16,36,44},255,cv::FILLED);
    cv::rectangle(truth,{76,24,28,36},255,cv::FILLED);
    cv::circle(truth,{32,38},8,0,cv::FILLED);
    cv::Mat scene=background.clone();
    cv::add(scene,cv::Scalar(60),scene,truth); // 目标比当地背景亮60。
    scene.at<uchar>(5,5)=255; // 已知为非目标的孤立亮点。
    std::cout << "truth pixels=" << cv::countNonZero(truth) << "; intended objects=2; hole=1\n";
    // [endregion]
    // [region methods]
    cv::Mat global,adaptive,difference,residual,opened;
    cv::threshold(scene,global,130,255,cv::THRESH_BINARY);
    cv::adaptiveThreshold(scene,adaptive,255,cv::ADAPTIVE_THRESH_MEAN_C,cv::THRESH_BINARY,31,-5);
    // 这一支拥有额外的已知背景参照；并非从输入图自动获得。
    cv::subtract(scene,background,difference);
    cv::threshold(difference,residual,30,255,cv::THRESH_BINARY);
    cv::morphologyEx(residual,opened,cv::MORPH_OPEN,cv::getStructuringElement(cv::MORPH_RECT,{3,3}));
    describe("global130",global,truth);
    describe("adaptive31",adaptive,truth);
    describe("referenceDiff",residual,truth);
    describe("referenceDiff+opening",opened,truth);
    // [endregion]
    // [region area]
    cv::Mat labels,stats,centroids;
    const int count=cv::connectedComponentsWithStats(residual,labels,stats,centroids,8);
    cv::Mat selected=cv::Mat::zeros(residual.size(),CV_8UC1);
    for(int id=1;id<count;++id)
        if(stats.at<int>(id,cv::CC_STAT_AREA)>=100)selected.setTo(255,labels==id);
    describe("referenceDiff+area100",selected,truth);
    // [endregion]
    cv::Mat edges;cv::Canny(scene,edges,30,80);
    std::cout << "Canny boundary pixels=" << cv::countNonZero(edges) << "; not scored as filled region\n";
}
