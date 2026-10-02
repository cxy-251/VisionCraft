#pragma once

#include "vc_protocol.h"

#include <opencv2/core.hpp>
#include <random>

// 模拟产线：生成一张传送带上的垫圈工件图，按概率随机注入一种缺陷。
// 缺陷是程序自己加的，所以每张图都有「标准答案」，可以统计检测算法的准确率。
class PartGenerator {
public:
    struct Part {
        cv::Mat image;          // BGR，kWidth × kHeight
        vc_defect truth;        // 注入的缺陷（VC_DEFECT_NONE = 合格品）
    };

    static constexpr int kWidth = 480;
    static constexpr int kHeight = 360;

    explicit PartGenerator(unsigned seed = std::random_device{}());

    // defectRate：出现缺陷的概率（0~1），出现时在四种缺陷里均匀挑一种
    // difficulty：缺陷有多不明显（0 = 很明显，1 = 接近检测阈值，会出现误判）
    Part next(double defectRate, double difficulty = 0.0);

    // 指定缺陷类型生成（测试用）
    Part make(vc_defect defect, double difficulty = 0.0);

private:
    std::mt19937 m_rng;
};
