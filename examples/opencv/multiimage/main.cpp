#include "MultiImageOps.h"
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <iostream>
int main(int argc,char **argv) {
    // [region fixed-cases]
    const std::filesystem::path output=argc>1 ? argv[1] : "";
    if(!output.empty()) std::filesystem::create_directories(output);
    for(int preset=0;preset<8;++preset) {
        teaching::MultiImageInput input; input.preset=preset; input.method=2;
        const auto result=teaching::composeLesson(input);
        std::cout<<"case "<<preset<<": "<<(result.error.empty()?result.summary:result.error)<<'\n';
        if(!result.error.empty()) return 1;
        if(!output.empty()) {
            // Keep just the input pair and final result as offline classroom materials.
            for(size_t i=0;i<result.images.size();++i) {
                if(preset!=7 && i>1 && i+1<result.images.size()) continue;
                const auto &view=result.images[i];
                if(!cv::imwrite((output/("case-"+std::to_string(preset)+"-"+view.first+".png")).string(),view.second)) return 2;
            }
        }
    }
    // [endregion]
    teaching::MultiImageInput input; input.estimateAlignment=true;
    const auto estimated=teaching::composeLesson(input);
    std::cout<<"estimated clean: "<<(estimated.error.empty()?estimated.summary:estimated.error)<<'\n';
    input.preset=5;
    std::cout<<"estimated low texture: "<<teaching::composeLesson(input).error<<'\n';
}
