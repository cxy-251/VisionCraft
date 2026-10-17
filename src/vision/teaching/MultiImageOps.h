#pragma once
#include <opencv2/core.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace teaching {
struct MultiImageInput {
    int preset = 0; // clean, exposure, black, parallax, motion, low texture, repeated texture, HDR
    int method = 1; // overwrite, feather, normalized multiband
    int levels = 4;
    bool estimateAlignment = false;
};
struct MultiImageResult {
    std::vector<std::pair<std::string,cv::Mat>> images;
    std::string summary, error;
    double overlapMae = 0;
    int overlapPixels = 0, inliers = 0;
    cv::Point canvasOrigin;
};
MultiImageResult composeLesson(const MultiImageInput &input,
                              const std::shared_ptr<std::atomic_bool> &cancel = {});
}
