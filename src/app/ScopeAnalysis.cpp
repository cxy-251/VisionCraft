#include "ScopeAnalysis.h"
#include <opencv2/core.hpp>
#include <cmath>

// [region spectrum]
QVariantMap ScopeAnalysis::spectrum(const QVariantMap &frame) const
{
    const auto invalid = [](const QString &why) {
        return QVariantMap{{"valid", false}, {"error", why}};
    };
    const QVariantList samples = frame.value("samples").toList();
    const int n = samples.size();
    bool rateOk = false, countOk = false;
    const double rate = frame.value("rate").toDouble(&rateOk);
    const int count = frame.value("count").toInt(&countOk);
    if (!rateOk || !countOk || !std::isfinite(rate) || rate < 10000 || rate > 200000 ||
        n < 32 || n > 2048 || count != n)
        return invalid(tr("等待有效的完整采样帧"));

    cv::Mat signal(n, 1, CV_64F);
    double mean = 0;
    for (int i = 0; i < n; ++i) {
        bool ok = false;
        const double value = samples[i].toDouble(&ok);
        if (!ok || !std::isfinite(value) || value < 0 || value > 4095 || std::floor(value) != value)
            return invalid(tr("采样帧含无效 ADC 码"));
        signal.at<double>(i) = value;
        mean += value;
    }
    mean /= n;
    double sumSquares = 0, windowSum = 0;
    const double pi = std::acos(-1.0);
    for (int i = 0; i < n; ++i) {
        const double ac = signal.at<double>(i) - mean;
        sumSquares += ac * ac;
        // 周期 Hann 窗；不补零，频率栅格为实际采样率 / 实际点数。
        const double window = .5 - .5 * std::cos(2 * pi * i / n);
        signal.at<double>(i) = ac * window;
        windowSum += window;
    }
    cv::Mat transformed;
    cv::dft(signal, transformed, cv::DFT_COMPLEX_OUTPUT);
    QVariantList hz, amplitudes;
    const double binHz = rate / n;
    double peak = 0;
    int peakBin = 0;
    for (int k = 0; k <= n / 2; ++k) {
        const auto z = transformed.at<cv::Vec2d>(k);
        const bool edge = k == 0 || (n % 2 == 0 && k == n / 2);
        const double amplitude = std::hypot(z[0], z[1]) * (edge ? 1.0 : 2.0) / windowSum;
        hz.append(k * binHz);
        amplitudes.append(amplitude);
        if (k > 0 && amplitude > peak) { peak = amplitude; peakBin = k; }
    }
    const double acRms = std::sqrt(sumSquares / n);
    // 不把恒定输入的浮点残差显示成主峰；单位均为 ADC 码，不是伏特。
    const bool hasPeak = acRms > 1e-9 && peak > 1e-9;
    return {{"valid", true}, {"hz", hz}, {"amplitudes", amplitudes},
            {"binHz", binHz}, {"nyquistHz", rate / 2}, {"mean", mean}, {"acRms", acRms},
            {"hasPeak", hasPeak}, {"peakHz", hasPeak ? peakBin * binHz : 0},
            {"peakAmplitude", hasPeak ? peak : 0}};
}
// [endregion]
