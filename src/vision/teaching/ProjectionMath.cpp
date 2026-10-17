#include "ProjectionMath.h"
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <cmath>
#include <stdexcept>
namespace teaching {
ProjectionResult projectScene(const ProjectionInput &p) {
    if (!std::isfinite(p.yawDegrees) || !std::isfinite(p.distanceMm) || !std::isfinite(p.cameraXMm)
        || !std::isfinite(p.focalPx) || p.focalPx <= 0 || p.imageSize.width < 1 || p.imageSize.height < 1
        || p.imageSize.width > 2048 || p.imageSize.height > 2048)
        throw std::invalid_argument("Invalid camera parameters");
    ProjectionResult result;
    const double cx = (p.imageSize.width - 1) / 2.0, cy = (p.imageSize.height - 1) / 2.0;
    result.intrinsics = cv::Matx33d(p.focalPx,0,cx, 0,p.focalPx,cy, 0,0,1);
    // [region coordinate-chain]
    cv::Matx33d objectRotation;
    cv::Rodrigues(cv::Vec3d(0, p.yawDegrees * CV_PI / 180, 0), objectRotation);
    const cv::Matx33d qtCameraToCv(1,0,0, 0,-1,0, 0,0,-1);
    const cv::Vec3d cameraInWorld(p.cameraXMm, 0, p.distanceMm);
    const std::vector<cv::Point3d> corners{{-100,-75,0},{100,-75,0},{100,75,0},{-100,75,0}};
    for (int layer = 0; layer < (p.secondDepth ? 2 : 1); ++layer) {
        for (auto point : corners) {
            point.z = layer * 100;
            const cv::Vec3d world = objectRotation * cv::Vec3d(point.x, point.y, point.z);
            const cv::Vec3d camera = qtCameraToCv * (world - cameraInWorld);
            ProjectedPoint out;
            out.object = point; out.world = cv::Point3d(world); out.camera = cv::Point3d(camera);
            out.inFront = camera[2] > 1e-6;
            if (out.inFront) {
                std::vector<cv::Point2d> pixels;
                cv::projectPoints(std::vector<cv::Point3d>{out.camera}, cv::Vec3d(), cv::Vec3d(),
                                  result.intrinsics, cv::Mat(), pixels);
                out.pixel = pixels.front();
                out.inside = out.pixel.x >= 0 && out.pixel.x < p.imageSize.width
                          && out.pixel.y >= 0 && out.pixel.y < p.imageSize.height;
            }
            result.points.push_back(out);
        }
    }
    // [endregion]
    // [region depth-parallax]
    // A horizontal camera translation: disparity differs between two fronto-parallel depths.
    // This scalar is a separate zero-yaw reference, not the general rotated-scene residual.
    if (p.distanceMm > 100)
        result.parallaxPx = std::abs(p.focalPx * p.cameraXMm
                           * (1.0 / (p.distanceMm - 100) - 1.0 / p.distanceMm));
    // [endregion]
    return result;
}
cv::Mat drawProjection(const ProjectionResult &r, cv::Size size) {
    cv::Mat image(size, CV_8UC3, cv::Scalar(32,28,24));
    for (int x = 0; x < size.width; x += 40) cv::line(image,{x,0},{x,size.height-1},{50,46,42});
    for (int y = 0; y < size.height; y += 40) cv::line(image,{0,y},{size.width-1,y},{50,46,42});
    for (size_t i = 0; i < r.points.size(); ++i) {
        const auto &p = r.points[i];
        const auto &next = r.points[(i / 4) * 4 + (i + 1) % 4];
        auto safe = [](const ProjectedPoint &q) {
            return q.inFront && std::abs(q.pixel.x) < 100000 && std::abs(q.pixel.y) < 100000;
        };
        const cv::Scalar color = i < 4 ? cv::Scalar(100,220,100) : cv::Scalar(70,160,255);
        if (safe(p) && safe(next)) cv::line(image,p.pixel,next.pixel,color,2,cv::LINE_AA);
        if (p.inside) {
            cv::circle(image,p.pixel,4,color,-1,cv::LINE_AA);
            cv::putText(image,std::to_string(i),p.pixel+cv::Point2d(7,-7),cv::FONT_HERSHEY_SIMPLEX,.5,color,1,cv::LINE_AA);
        }
    }
    return image;
}
}
