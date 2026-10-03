#include "PartGenerator.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

PartGenerator::PartGenerator(unsigned seed)
    : m_rng(seed)
{
}

PartGenerator::Part PartGenerator::next(double defectRate, double difficulty)
{
    std::uniform_real_distribution<double> u(0.0, 1.0);
    if (u(m_rng) >= defectRate)
        return make(VC_DEFECT_NONE, difficulty);
    std::uniform_int_distribution<int> pick(VC_DEFECT_SCRATCH, VC_DEFECT_OFFSET);
    return make(static_cast<vc_defect>(pick(m_rng)), difficulty);
}

PartGenerator::Part PartGenerator::make(vc_defect defect, double difficulty)
{
    auto uni = [this](double a, double b) { return std::uniform_real_distribution<double>(a, b)(m_rng); };
    const double k = std::clamp(difficulty, 0.0, 1.0);
    auto lerp = [k](double easy, double hard) { return easy + (hard - easy) * k; };

    // [region belt]
    // ---- 传送带：深灰，带横向条纹 ----
    cv::Mat img(kHeight, kWidth, CV_8UC3);
    for (int y = 0; y < kHeight; ++y) {
        const int v = 52 + int(6 * std::sin(y * 0.35));
        img.row(y).setTo(cv::Scalar(v, v, v + 4));
    }

    // [endregion]
    // [region washer]
    // ---- 垫圈：外圆 + 内孔，位置和大小有小幅随机 ----
    const cv::Point2d c(kWidth / 2.0 + uni(-14, 14), kHeight / 2.0 + uni(-10, 10));
    const double R = uni(112, 118);
    const double r = uni(44, 48);
    const cv::Scalar metal(196, 198, 200);
    cv::circle(img, c, int(std::lround(R)), metal, cv::FILLED, cv::LINE_AA);

    // 金属表面的径向明暗：在 1/4 分辨率上画一个亮斑再模糊、放大，比在原图上做大半径模糊快几十倍
    cv::Mat small(img.rows / 4, img.cols / 4, CV_8UC3, cv::Scalar(0, 0, 0));
    cv::circle(small, (c + cv::Point2d(-R * 0.3, -R * 0.3)) / 4, int(R * 0.7 / 4), cv::Scalar(18, 18, 18), cv::FILLED);
    cv::GaussianBlur(small, small, cv::Size(0, 0), 6);
    cv::Mat shade;
    cv::resize(small, shade, img.size(), 0, 0, cv::INTER_LINEAR);
    cv::Mat ringMask(img.size(), CV_8UC1, cv::Scalar(0));
    cv::circle(ringMask, c, int(std::lround(R)), cv::Scalar(255), cv::FILLED, cv::LINE_AA);
    cv::add(img, shade, img, ringMask);

    cv::Point2d hole = c;
    if (defect == VC_DEFECT_OFFSET) {
        const double a = uni(0, 2 * kPi), d = lerp(uni(12, 20), uni(4, 9));   // 难：判定阈值是 6 px
        hole += cv::Point2d(d * std::cos(a), d * std::sin(a));
    }
    // 内孔里露出的是传送带
    cv::Mat holeMask(img.size(), CV_8UC1, cv::Scalar(0));
    cv::circle(holeMask, hole, int(std::lround(r)), cv::Scalar(255), cv::FILLED, cv::LINE_AA);
    for (int y = 0; y < kHeight; ++y) {
        const int v = 52 + int(6 * std::sin(y * 0.35));
        img.row(y).setTo(cv::Scalar(v, v, v + 4), holeMask.row(y));
    }

    // [endregion]
    // [region defects]
    // ---- 缺陷 ----
    switch (defect) {
    case VC_DEFECT_SCRATCH: {
        // 划痕：环面上一条细长的暗线
        const double a = uni(0, 2 * kPi), rr = uni(r + 20, R - 22);
        const cv::Point2d mid = c + cv::Point2d(rr * std::cos(a), rr * std::sin(a));
        const double dir = a + kPi / 2 + uni(-0.5, 0.5), len = lerp(uni(26, 42), uni(12, 24));
        const cv::Point2d d(std::cos(dir) * len / 2, std::sin(dir) * len / 2);
        const double v = lerp(105, 160);   // 难：划痕更浅，接近金属本色
        cv::line(img, mid - d, mid + d, cv::Scalar(v, v, v + 3), k > 0.5 ? 1 : 2, cv::LINE_AA);
        break;
    }
    case VC_DEFECT_CHIP: {
        // 缺口：外边缘被啃掉一块（露出传送带）
        const double a = uni(0, 2 * kPi), cr = lerp(uni(14, 22), uni(5, 10));
        const cv::Point2d p = c + cv::Point2d(R * std::cos(a), R * std::sin(a));
        cv::Mat chip(img.size(), CV_8UC1, cv::Scalar(0));
        cv::circle(chip, p, int(cr), cv::Scalar(255), cv::FILLED, cv::LINE_AA);
        for (int y = 0; y < kHeight; ++y) {
            const int v = 52 + int(6 * std::sin(y * 0.35));
            img.row(y).setTo(cv::Scalar(v, v, v + 4), chip.row(y));
        }
        break;
    }
    case VC_DEFECT_SPOT: {
        // 污点：环面上一块小的暗斑
        const double a = uni(0, 2 * kPi), rr = uni(r + 16, R - 16);
        const cv::Point2d p = c + cv::Point2d(rr * std::cos(a), rr * std::sin(a));
        const double v = lerp(90, 150);
        cv::ellipse(img, p, cv::Size(int(lerp(uni(5, 8), uni(2, 4))), int(lerp(uni(4, 7), uni(2, 3)))), uni(0, 180), 0, 360,
                    cv::Scalar(v + 2, v - 2, v - 6), cv::FILLED, cv::LINE_AA);
        break;
    }
    default:
        break;
    }

    // [endregion]
    // [region marks]
    // ---- 无害的加工痕迹：每件都可能有几道很浅的短纹，它们不算缺陷 ----
    // 真实工件的表面不会完美干净。没有这些痕迹，阈值调得再低也不会误报，「漏检和误报此消彼长」就体现不出来
    std::uniform_int_distribution<int> markCount(0, 4);
    for (int i = markCount(m_rng); i > 0; --i) {
        const double a = uni(0, 2 * kPi), rr = uni(r + 14, R - 14);
        const cv::Point2d mid = c + cv::Point2d(rr * std::cos(a), rr * std::sin(a));
        const double dir = uni(0, kPi), len = uni(5, 12);
        const cv::Point2d d(std::cos(dir) * len / 2, std::sin(dir) * len / 2);
        const double v = 196 - uni(12, 30);   // 比金属本色暗 12~30 级，比真缺陷浅得多
        cv::line(img, mid - d, mid + d, cv::Scalar(v, v, v), 1, cv::LINE_AA);
    }

    // [endregion]
    // [region noise]
    // ---- 成像噪声：轻微模糊 + 高斯噪声 ----
    cv::GaussianBlur(img, img, cv::Size(3, 3), 0.8);
    cv::Mat noise(img.size(), CV_16SC3);
    cv::randn(noise, cv::Scalar::all(0), cv::Scalar::all(4));
    cv::Mat img16;
    img.convertTo(img16, CV_16SC3);
    img16 += noise;
    img16.convertTo(img, CV_8UC3);

    // [endregion]
    return {img, defect};
}
