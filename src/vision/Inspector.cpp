#include "Inspector.h"

#include <QElapsedTimer>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

QString Inspector::defectName(vc_defect d)
{
    switch (d) {
    case VC_DEFECT_NONE: return QStringLiteral("合格");
    case VC_DEFECT_SCRATCH: return QStringLiteral("划痕");
    case VC_DEFECT_CHIP: return QStringLiteral("缺口");
    case VC_DEFECT_SPOT: return QStringLiteral("污点");
    case VC_DEFECT_OFFSET: return QStringLiteral("内孔偏心");
    case VC_DEFECT_MISSING: return QStringLiteral("未找到工件");
    default: return QStringLiteral("未知");
    }
}

Inspector::Result Inspector::inspect(const cv::Mat &bgr, Steps *steps) const
{
    auto keep = [steps](const char *name, const cv::Mat &m) {
        if (steps)
            steps->emplace_back(QString::fromUtf8(name), m.clone());
    };
    QElapsedTimer clock;
    clock.start();
    Result res;
    res.annotated = bgr.clone();
    const cv::Scalar red(60, 60, 240), green(90, 210, 90), yellow(40, 200, 240);

    auto finish = [&](vc_defect d) {
        res.defect = d;
        res.ok = d == VC_DEFECT_NONE;
        res.ms = clock.nsecsElapsed() / 1e6;
        res.measures[QStringLiteral("ms")] = res.ms;
        cv::putText(res.annotated, res.ok ? "OK" : "NG", cv::Point(16, 44), cv::FONT_HERSHEY_SIMPLEX, 1.3,
                    res.ok ? green : red, 3, cv::LINE_AA);
        return res;
    };

    // [region segment]
    // ---- 1. 分割：工件比传送带亮得多，大津法自动选阈值 ----
    cv::Mat gray, blurred, bin;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
    const double otsu = cv::threshold(blurred, bin, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    res.measures[QStringLiteral("otsu")] = otsu;
    keep("gray", gray);
    keep("blurred", blurred);
    keep("binary", bin);

    // ---- 2. 轮廓：外轮廓是最大的那个，它的子轮廓里最大的是内孔 ----
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(bin, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_NONE);
    int outer = -1;
    double outerArea = 0;
    for (int i = 0; i < int(contours.size()); ++i) {
        if (hierarchy[i][3] != -1)
            continue;   // 只看最外层
        const double a = cv::contourArea(contours[i]);
        if (a > outerArea) {
            outerArea = a;
            outer = i;
        }
    }
    if (outer < 0 || outerArea < 5000)
        return finish(VC_DEFECT_MISSING);

// [endregion]
    int hole = -1;
    double holeArea = 0;
    for (int child = hierarchy[outer][2]; child >= 0; child = hierarchy[child][0]) {
        const double a = cv::contourArea(contours[child]);
        if (a > holeArea) {
            holeArea = a;
            hole = child;
        }
    }
    if (hole < 0 || holeArea < 1000)
        return finish(VC_DEFECT_MISSING);

    // ---- 3. 外圆和内孔：用最小二乘拟合圆心 ----
    // 外圆不用最小外接圆：缺口不影响外接圆，但会拉偏拟合；这里用外轮廓的凸包拟合，缺口处凸包直接跨过去
    std::vector<cv::Point> hull;
    cv::convexHull(contours[outer], hull);
    const cv::RotatedRect outerFit = cv::fitEllipse(hull);
    const cv::RotatedRect holeFit = cv::fitEllipse(contours[hole]);
    const cv::Point2f oc = outerFit.center, hc = holeFit.center;
    const double outerR = (outerFit.size.width + outerFit.size.height) / 4.0;
    const double holeR = (holeFit.size.width + holeFit.size.height) / 4.0;
    const double offset = cv::norm(oc - hc);
    res.measures[QStringLiteral("outerR")] = outerR;
    res.measures[QStringLiteral("holeR")] = holeR;
    res.measures[QStringLiteral("centerOffset")] = offset;

    cv::circle(res.annotated, oc, int(std::lround(outerR)), green, 1, cv::LINE_AA);
    cv::circle(res.annotated, hc, int(std::lround(holeR)), green, 1, cv::LINE_AA);
    cv::drawMarker(res.annotated, oc, green, cv::MARKER_CROSS, 10);
    cv::drawMarker(res.annotated, hc, yellow, cv::MARKER_CROSS, 10);

    // [region chip]
    // ---- 4. 缺口：外轮廓的凸缺陷（轮廓凹进去的地方）有多深 ----
    std::vector<int> hullIdx;
    cv::convexHull(contours[outer], hullIdx, false, false);
    double chipDepth = 0;
    cv::Point chipAt;
    if (hullIdx.size() > 3) {
        std::sort(hullIdx.begin(), hullIdx.end());   // convexityDefects 要求索引单调
        std::vector<cv::Vec4i> defects;
        cv::convexityDefects(contours[outer], hullIdx, defects);
        for (const cv::Vec4i &d : defects) {
            const double depth = d[3] / 256.0;   // 定点数，低 8 位是小数
            if (depth > chipDepth) {
                chipDepth = depth;
                chipAt = contours[outer][d[2]];
            }
        }
    }
    res.measures[QStringLiteral("chipDepth")] = chipDepth;
    if (chipDepth > params.minChipDepth) {
        cv::circle(res.annotated, chipAt, 22, red, 2, cv::LINE_AA);
        return finish(VC_DEFECT_CHIP);
    }
// [endregion]

    if (offset > params.maxCenterOffset) {
        cv::line(res.annotated, oc, hc, red, 2, cv::LINE_AA);
        return finish(VC_DEFECT_OFFSET);
    }

    // [region surface]
    // ---- 5. 表面：只看环面，离两条边各留几个像素，免得边缘被当成缺陷 ----
    cv::Mat ring(gray.size(), CV_8UC1, cv::Scalar(0));
    cv::circle(ring, oc, int(outerR - 7), cv::Scalar(255), cv::FILLED);
    cv::circle(ring, hc, int(holeR + 7), cv::Scalar(0), cv::FILLED);

    // 黑帽 = 闭运算 - 原图：把「比周围暗的小东西」提出来，同时消掉大面积的明暗变化
    cv::Mat blackhat;
    cv::morphologyEx(blurred, blackhat, cv::MORPH_BLACKHAT, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(21, 21)));
    cv::Mat suspect;
    cv::threshold(blackhat, suspect, params.surfaceThreshold, 255, cv::THRESH_BINARY);
    keep("ring", ring);
    keep("blackhat", blackhat);
    keep("suspect-raw", suspect);
    suspect &= ring;
    keep("suspect", suspect);

    cv::Mat labels, stats, centroids;
    const int n = cv::connectedComponentsWithStats(suspect, labels, stats, centroids, 8);
    int worst = -1;
    int worstArea = 0;
    for (int i = 1; i < n; ++i) {
        const int a = stats.at<int>(i, cv::CC_STAT_AREA);
        if (a >= params.minDefectArea && a > worstArea) {
            worstArea = a;
            worst = i;
        }
    }
    res.measures[QStringLiteral("surfaceArea")] = worstArea;
    if (worst < 0)
        return finish(VC_DEFECT_NONE);

    // 划痕和污点的区别：形状。用最小外接旋转矩形的长宽比
    std::vector<cv::Point> pts;
    cv::findNonZero(labels == worst, pts);
    const cv::RotatedRect box = cv::minAreaRect(pts);
    const double longSide = std::max(box.size.width, box.size.height);
    const double shortSide = std::max(1.0f, std::min(box.size.width, box.size.height));
    const double elongation = longSide / shortSide;
    res.measures[QStringLiteral("elongation")] = elongation;

    cv::Point2f corners[4];
    box.points(corners);
    for (int i = 0; i < 4; ++i)
        cv::line(res.annotated, corners[i], corners[(i + 1) % 4], red, 2, cv::LINE_AA);
    return finish(elongation > params.scratchElongation ? VC_DEFECT_SCRATCH : VC_DEFECT_SPOT);
// [endregion]
}
