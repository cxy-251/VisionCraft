#pragma once
#include <opencv2/core.hpp>
#include <vector>
namespace teaching {
// [region camera-contract]
struct ProjectionInput {
    double yawDegrees = 20;
    double distanceMm = 500;
    double cameraXMm = 0;
    double focalPx = 500;
    bool secondDepth = false;
    cv::Size imageSize{640, 480};
};
struct ProjectedPoint {
    cv::Point3d object, world, camera;
    cv::Point2d pixel;
    bool inFront = false, inside = false;
};
struct ProjectionResult {
    std::vector<ProjectedPoint> points;
    cv::Matx33d intrinsics;
    double parallaxPx = 0;
};
ProjectionResult projectScene(const ProjectionInput &input);
cv::Mat drawProjection(const ProjectionResult &result, cv::Size size);
// [endregion]
}
