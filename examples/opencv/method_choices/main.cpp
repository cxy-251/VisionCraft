#include <opencv2/imgproc.hpp>
#include <iostream>
static int inside(const cv::Mat &binary,const cv::Mat &mask) {
    cv::Mat intersection;cv::bitwise_and(binary,mask,intersection);return cv::countNonZero(intersection);
}
int main() {
    // [region segmentation]
    cv::Mat scene(48,80,CV_8UC1),truth=cv::Mat::zeros(scene.size(),CV_8UC1);
    for(int y=0;y<scene.rows;++y)for(int x=0;x<scene.cols;++x)scene.at<uchar>(y,x)=uchar(30+2*x);
    const cv::Rect object(10,10,15,18);
    scene(object)+=50;truth(object).setTo(255);
    cv::Mat global,adaptive,edges;
    cv::threshold(scene,global,95,255,cv::THRESH_BINARY);
    cv::adaptiveThreshold(scene,adaptive,255,cv::ADAPTIVE_THRESH_MEAN_C,cv::THRESH_BINARY,15,-8);
    cv::Canny(scene,edges,30,80);
    std::cout << "[segmentation] object pixels=" << cv::countNonZero(truth) << '\n';
    for(const auto &entry:std::vector<std::pair<const char*,cv::Mat>>{{"global",global},{"adaptive",adaptive}}) {
        const int hit=inside(entry.second,truth);
        std::cout << entry.first << ": inside=" << hit << " outside=" << cv::countNonZero(entry.second)-hit << '\n';
    }
    std::cout << "Canny boundary pixels=" << cv::countNonZero(edges) << " (not a region mask)\n";
    // [endregion]
    // [region filtering]
    cv::Mat binary=cv::Mat::zeros(64,64,CV_8UC1);
    cv::rectangle(binary,{20,20,24,24},255,cv::FILLED);
    binary.at<uchar>(32,32)=0;binary.at<uchar>(5,5)=255;
    cv::line(binary,{10,8},{40,8},255,1);
    const auto kernel=cv::getStructuringElement(cv::MORPH_RECT,{3,3});
    cv::Mat blur,thresholded,median,opened,closed;
    cv::GaussianBlur(binary,blur,{3,3},0);
    cv::threshold(blur,thresholded,128,255,cv::THRESH_BINARY);
    cv::medianBlur(binary,median,3);
    cv::morphologyEx(binary,opened,cv::MORPH_OPEN,kernel);
    cv::morphologyEx(binary,closed,cv::MORPH_CLOSE,kernel);
    std::cout << "[filtering] Gaussian intermediate gray=" << cv::countNonZero((blur!=0)&(blur!=255)) << '\n';
    for(const auto &entry:std::vector<std::pair<const char*,cv::Mat>>{
            {"input",binary},{"Gaussian+threshold",thresholded},{"median",median},{"opening",opened},{"closing",closed}}) {
        const auto &image=entry.second;
        std::cout << entry.first << ": count=" << cv::countNonZero(image)
                  << " dot=" << int(image.at<uchar>(5,5)) << " thinLine=" << int(image.at<uchar>(8,25))
                  << " hole=" << int(image.at<uchar>(32,32)) << '\n';
    }
    // [endregion]
    // [region regions]
    cv::Mat mask=cv::Mat::zeros(20,30,CV_8UC1);
    cv::rectangle(mask,{2,2,5,5},255,cv::FILLED);mask.at<uchar>(4,4)=0;
    mask.at<uchar>(3,12)=255;mask.at<uchar>(12,17)=255;mask.at<uchar>(13,18)=255;
    cv::Mat labels,stats,centroids;
    std::cout << "[regions] foreground=" << cv::countNonZero(mask) << '\n';
    for(int connectivity:{4,8}) {
        const int count=cv::connectedComponentsWithStats(mask,labels,stats,centroids,connectivity);
        std::cout << connectivity << "-connected objects=" << count-1 << '\n';
    }
    std::vector<std::vector<cv::Point>> contours;std::vector<cv::Vec4i> hierarchy;
    cv::findContours(mask,contours,hierarchy,cv::RETR_TREE,cv::CHAIN_APPROX_SIMPLE);
    double outerArea=0;int outerCount=0,holeCount=0;
    for(size_t i=0;i<contours.size();++i) {
        if(hierarchy[i][3]<0){++outerCount;outerArea+=cv::contourArea(contours[i]);}else ++holeCount;
    }
    const int mainLabel=labels.at<int>(2,2);
    std::cout << "outer contours=" << outerCount << " child contours=" << holeCount
              << " sum outer polygon area=" << outerArea
              << " main component pixels=" << stats.at<int>(mainLabel,cv::CC_STAT_AREA) << '\n';
    // [endregion]
}
