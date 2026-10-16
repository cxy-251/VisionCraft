#pragma once
#include <QString>
#include <QVariantMap>
#include <opencv2/core.hpp>

// 旧实验室运算的新界面适配；保持在视觉层，不读取控件、不操作设备。
namespace LabAlgorithms {
struct Result { cv::Mat image; QVariantMap values; QString error; };
Result match(const cv::Mat &source, const cv::Mat &pattern, double threshold);
Result stream(const cv::Mat &source, const QVariantMap &options);
Result node(const cv::Mat &source, const QString &type, const QVariantMap &options);
cv::Mat mountingSample(int index);
cv::Mat graphSample(bool defect);
struct MountingResult {
    cv::Mat image;
    bool passed=false;
    double diameter=0, area=0, ms=0;
    QString reason;
};
MountingResult inspectMounting(const cv::Mat &source, double tolerance);
}
