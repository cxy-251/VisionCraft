// 频域滤波（去掉传送带的周期条纹）、二维码与条码
//
// 运行：./example_opencv_freq_codes <输出目录>     输出见 output.txt

#include "PartGenerator.h"

#include <QDir>
#include <QString>
#include <cmath>
#include <cstdio>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/objdetect/barcode.hpp>

static QString g_dir;
static void save(const char *n, const cv::Mat &m) { cv::imwrite((g_dir + "/" + n + ".png").toStdString(), m); }

// 传送带区域（左上角一块，没有垫圈）的条纹起伏：每行平均亮度的最大值 − 最小值
static double stripeAmplitude(const cv::Mat &gray)
{
    cv::Mat rows;
    cv::reduce(gray(cv::Rect(0, 0, 80, 360)), rows, 1, cv::REDUCE_AVG, CV_64F);
    double lo, hi;
    cv::minMaxLoc(rows, &lo, &hi);
    return hi - lo;
}

// EAN-13 条码：前置数字决定左边 6 位用 L 还是 G 编码，右边 6 位用 R 编码
static cv::Mat ean13(const std::string &digits, int module = 3)
{
    static const char *L[] = {"0001101", "0011001", "0010011", "0111101", "0100011", "0110001", "0101111", "0111011", "0110111", "0001011"};
    static const char *G[] = {"0100111", "0110011", "0011011", "0100001", "0011101", "0111001", "0000101", "0010001", "0001001", "0010111"};
    static const char *R[] = {"1110010", "1100110", "1101100", "1000010", "1011100", "1001110", "1010000", "1000100", "1001000", "1110100"};
    static const char *parity[] = {"LLLLLL", "LLGLGG", "LLGGLG", "LLGGGL", "LGLLGG", "LGGLLG", "LGGGLL", "LGLGLG", "LGLGGL", "LGGLGL"};
    std::string bits = "101";
    for (int i = 1; i <= 6; ++i)
        bits += (parity[digits[0] - '0'][i - 1] == 'L' ? L : G)[digits[i] - '0'];
    bits += "01010";
    for (int i = 7; i <= 12; ++i)
        bits += R[digits[i] - '0'];
    bits += "101";
    const int quiet = 12 * module;     // 两侧静区至少 11 个模块宽
    cv::Mat img(220, int(bits.size()) * module + 2 * quiet, CV_8UC1, cv::Scalar(255));   // 上下也留白
    for (size_t i = 0; i < bits.size(); ++i)
        if (bits[i] == '1')
            img(cv::Rect(quiet + int(i) * module, 55, module, 110)).setTo(0);
    return img;
}

int main(int argc, char *argv[])
{
    g_dir = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QStringLiteral(".");
    QDir().mkpath(g_dir);

    std::printf("==== 1. 频域滤波：去掉传送带的横条纹 ====\n");
    PartGenerator gen(20261003);
    cv::Mat gray;
    cv::cvtColor(gen.make(VC_DEFECT_SCRATCH, 0.0).image, gray, cv::COLOR_BGR2GRAY);
    // [region dft]
    cv::Mat f;
    gray.convertTo(f, CV_32F);
    cv::Mat spectrum;
    cv::dft(f, spectrum, cv::DFT_COMPLEX_OUTPUT);           // 每个点是一个复数：某个频率、某个方向的正弦波有多强
    // 条纹是竖直方向（沿 y）的正弦波，周期约 18 行：频率 360 / 18 ≈ 20，落在频谱第 0 列、第 ±20 行附近
    cv::Mat mag;
    std::vector<cv::Mat> ch;
    cv::split(spectrum, ch);
    cv::magnitude(ch[0], ch[1], mag);
    int peakRow = 0;
    float peak = 0;
    for (int v = 5; v < 60; ++v)                             // 跳过最低频（整体亮度、大块明暗）
        if (mag.at<float>(v, 0) > peak) { peak = mag.at<float>(v, 0); peakRow = v; }
    std::printf("  频谱第 0 列（纯竖直方向的波）在第 %d 行有一个峰：周期 %.1f 行\n", peakRow, 360.0 / peakRow);
    // [endregion]
    // [region notch]
    // 陷波：只把这个频率（和它对称的负频率）附近置零，其余不动
    cv::Mat notch(spectrum.size(), CV_32FC2, cv::Scalar(1, 1));
    for (int v : {peakRow, gray.rows - peakRow})
        for (int dv = -2; dv <= 2; ++dv)
            for (int u : {0, 1, 2, gray.cols - 1, gray.cols - 2})
                notch.at<cv::Vec2f>(v + dv, u) = cv::Vec2f(0, 0);
    cv::Mat filtered;
    cv::multiply(spectrum, notch, filtered);
    cv::Mat back;
    cv::idft(filtered, back, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);
    cv::Mat result;
    back.convertTo(result, CV_8U);
    // [endregion]
    std::printf("  传送带条纹起伏：原图 %.1f → 陷波后 %.1f\n", stripeAmplitude(gray), stripeAmplitude(result));
    cv::Mat diff;
    cv::absdiff(gray, result, diff);
    std::printf("  垫圈区域（中心 100×100）平均变化 %.2f\n", cv::mean(diff(cv::Rect(190, 130, 100, 100)))[0]);
    // [region blur-compare]
    cv::Mat blurred;
    cv::GaussianBlur(gray, blurred, cv::Size(0, 0), 6);      // 对比：用足以抹掉条纹的高斯模糊
    cv::absdiff(gray, blurred, diff);
    std::printf("  对比：sigma 6 的高斯模糊，条纹起伏 %.1f，垫圈区域平均变化 %.2f\n", stripeAmplitude(blurred),
                cv::mean(diff(cv::Rect(190, 130, 100, 100)))[0]);
    // [endregion]
    cv::Mat logMag, shownMag;
    cv::log(mag + 1, logMag);
    cv::normalize(logMag, shownMag, 0, 255, cv::NORM_MINMAX, CV_8U);
    // 把零频移到中心再存图，看起来才是常见的频谱样子
    const int cx = shownMag.cols / 2, cy = shownMag.rows / 2;
    cv::Mat q0(shownMag, {0, 0, cx, cy}), q1(shownMag, {cx, 0, cx, cy}), q2(shownMag, {0, cy, cx, cy}), q3(shownMag, {cx, cy, cx, cy}), tmp;
    q0.copyTo(tmp); q3.copyTo(q0); tmp.copyTo(q3);
    q1.copyTo(tmp); q2.copyTo(q1); tmp.copyTo(q2);
    save("freq-input", gray);
    save("freq-spectrum", shownMag);
    save("freq-notched", result);

    std::printf("\n==== 2. 二维码 ====\n");
    // [region qr]
    cv::Ptr<cv::QRCodeEncoder> enc = cv::QRCodeEncoder::create();
    cv::Mat qr;
    enc->encode("VC-0042|LOT7|2026-10-04", qr);              // 每个模块一个像素
    cv::Mat big;
    cv::resize(qr, big, {}, 6, 6, cv::INTER_NEAREST);         // 放大到每个模块 6 像素
    cv::copyMakeBorder(big, big, 24, 24, 24, 24, cv::BORDER_CONSTANT, cv::Scalar(255));   // 四周留白（静区）
    cv::QRCodeDetector det;
    std::printf("  二维码 %dx%d 个模块；正放：「%s」\n", qr.cols, qr.rows, det.detectAndDecode(big).c_str());
    // [endregion]
    save("qr", big);
    // [region qr-stress]
    for (double angle : {30.0, 75.0}) {
        cv::Mat rot;
        cv::warpAffine(big, rot, cv::getRotationMatrix2D({big.cols / 2.f, big.rows / 2.f}, angle, 0.8), big.size(),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255));
        std::printf("  旋转 %.0f°：「%s」\n", angle, det.detectAndDecode(rot).c_str());
    }
    for (int module : {3, 2, 1}) {
        cv::Mat small;
        cv::resize(qr, small, {}, module, module, cv::INTER_NEAREST);
        cv::copyMakeBorder(small, small, 4 * module, 4 * module, 4 * module, 4 * module, cv::BORDER_CONSTANT, cv::Scalar(255));
        cv::Mat scene(200, 200, CV_8UC1, cv::Scalar(255));
        small.copyTo(scene(cv::Rect(10, 10, small.cols, small.rows)));
        std::printf("  每个模块 %d 像素：「%s」\n", module, det.detectAndDecode(scene).c_str());
    }
    {
        cv::Mat noQuiet;
        cv::resize(qr, noQuiet, {}, 6, 6, cv::INTER_NEAREST);
        cv::Mat scene(noQuiet.rows + 40, noQuiet.cols + 40, CV_8UC1);
        cv::RNG(4).fill(scene, cv::RNG::UNIFORM, 0, 255);      // 四周是杂乱的纹理，没有留白
        noQuiet.copyTo(scene(cv::Rect(20, 20, noQuiet.cols, noQuiet.rows)));
        std::printf("  四周没有留白、是杂乱纹理：「%s」\n", det.detectAndDecode(scene).c_str());
    }
    // [endregion]

    std::printf("\n==== 3. 条码 EAN-13 ====\n");
    // [region barcode]
    cv::barcode::BarcodeDetector bdet;
    auto decode = [&](const char *what, const cv::Mat &img) {
        std::vector<std::string> info, type;
        std::vector<cv::Point2f> corners;
        bdet.detectAndDecodeWithType(img, info, type, corners);
        std::printf("  %-34s 找到 %zu 个", what, info.size());
        for (size_t i = 0; i < info.size(); ++i)
            std::printf("，类型 %s，内容「%s」", type[i].c_str(), info[i].c_str());
        std::printf("\n");
    };
    auto blur = [](const cv::Mat &m) { cv::Mat b; cv::GaussianBlur(m, b, {3, 3}, 0.8); return b; };   // 相机拍出来总有一点模糊
    decode("每条 2 像素宽，边缘完全锐利：", ean13("6901234567892", 2));
    decode("每条 3 像素宽，边缘完全锐利：", ean13("6901234567892", 3));
    decode("每条 3 像素宽，轻微模糊：", blur(ean13("6901234567892", 3)));
    decode("校验位写错（…890），轻微模糊：", blur(ean13("6901234567890", 3)));
    // [endregion]
    save("barcode", ean13("6901234567892", 3));
    return 0;
}
