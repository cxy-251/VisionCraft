#include "ProjectionMath.h"
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <iomanip>
#include <iostream>
int main(int argc,char **argv) {
    teaching::ProjectionInput input;
    input.yawDegrees=0; input.cameraXMm=80; input.secondDepth=true;
    const auto result=teaching::projectScene(input);
    // [region coordinates]
    std::cout<<std::fixed<<std::setprecision(2);
    for(size_t i=0;i<result.points.size();++i) {
        const auto &p=result.points[i];
        std::cout<<"point "<<i<<" object="<<p.object<<" world="<<p.world<<" camera="<<p.camera
                 <<" pixel="<<p.pixel<<" inFront="<<p.inFront<<" inside="<<p.inside<<'\n';
    }
    std::cout<<"zero-yaw translation residual between Z=500 and Z=400: "<<result.parallaxPx<<" px\n";
    // [endregion]
    if(argc>1) {
        std::filesystem::create_directories(argv[1]);
        if(!cv::imwrite((std::filesystem::path(argv[1])/"two-depths.png").string(),teaching::drawProjection(result,input.imageSize))) return 1;
    }
    input.distanceMm=50;
    const auto near=teaching::projectScene(input);
    int behind=0; for(const auto &p:near.points) behind+=!p.inFront;
    std::cout<<"camera at world Z=50: "<<behind<<" points rejected before division\n";
}
