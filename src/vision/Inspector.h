#pragma once

#include "vc_protocol.h"

#include <QString>
#include <QVariantMap>
#include <opencv2/core.hpp>

// 垫圈检测：找到工件 → 量外圆和内孔 → 查边缘缺口和偏心 → 查环面上的划痕和污点。
// 输入一张 BGR 图（来自工位页面的截图），输出判定、缺陷类型、标注图和各项测量值。
class Inspector {
public:
    struct Result {
        bool ok = false;
        vc_defect defect = VC_DEFECT_MISSING;
        double ms = 0;               // 检测耗时
        cv::Mat annotated;           // 画了测量结果的图
        QVariantMap measures;        // 中间测量值，界面上展示
    };

    // 判定阈值（单位：像素）。放在这里方便在手册里讲「阈值怎么定」
    struct Params {
        double maxCenterOffset = 6.0;     // 内孔圆心与外圆圆心距离超过它算偏心
        double minChipDepth = 5.0;        // 外轮廓凸缺陷深度超过它算缺口
        int    surfaceThreshold = 40;     // 黑帽变换后亮度差超过它算表面异常
        int    minDefectArea = 12;        // 表面异常面积小于它忽略（噪声）
        double scratchElongation = 3.0;   // 长宽比超过它算划痕，否则算污点
    };

    Result inspect(const cv::Mat &bgr) const;

    Params params;

    static QString defectName(vc_defect d);   // 中文名
};
