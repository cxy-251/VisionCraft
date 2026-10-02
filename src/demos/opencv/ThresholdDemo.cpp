#include "ThresholdDemo.h"

#include "PartGenerator.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>

ThresholdDemo::ThresholdDemo(QObject *parent)
    : QObject(parent)
{
    newPart();
    m_threshold = m_otsu;
    apply();
}

void ThresholdDemo::newPart()
{
    PartGenerator gen(m_seed++);
    cv::cvtColor(gen.make(VC_DEFECT_NONE).image, m_partGray, cv::COLOR_BGR2GRAY);
    analyse();
}

void ThresholdDemo::setLighting(double l)
{
    m_lighting = l;
    analyse();
}

void ThresholdDemo::analyse()
{
    // 光照不均：左边有一片眩光（整体加亮），右边越来越暗（整体乘一个系数）。
    // 只是整体变暗的话，工件和背景同比例变暗，全局阈值照样分得开；
    // 真正的麻烦是眩光把一侧的背景抬得比另一侧的工件还亮
    m_gray = m_partGray.clone();
    if (m_lighting > 0) {
        for (int x = 0; x < m_gray.cols; ++x) {
            const double t = double(x) / (m_gray.cols - 1);
            const double gain = 1.0 - 0.6 * m_lighting * t;
            const double glare = 110.0 * m_lighting * (1.0 - t);
            m_gray.col(x).convertTo(m_gray.col(x), -1, gain, glare);
        }
    }
    // [region otsu]
    cv::Mat dummy;
    m_otsu = int(cv::threshold(m_gray, dummy, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU));
    // [endregion]

    // [region histogram]
    cv::Mat hist;
    const int channels[] = {0};
    const int histSize[] = {256};
    const float range[] = {0, 256};
    const float *ranges[] = {range};
    cv::calcHist(&m_gray, 1, channels, cv::Mat(), hist, 1, histSize, ranges);
    // [endregion]
    m_hist.clear();
    for (int i = 0; i < 256; ++i)
        m_hist << int(hist.at<float>(i));

    m_source = QImage(m_gray.data, m_gray.cols, m_gray.rows, int(m_gray.step), QImage::Format_Grayscale8).copy();
    emit partChanged();
    apply();
}

void ThresholdDemo::setThreshold(int t)
{
    m_threshold = std::clamp(t, 0, 255);
    apply();
}

void ThresholdDemo::apply()
{
    cv::Mat bin;
    // [region threshold]
    cv::threshold(m_gray, bin, m_threshold, 255, cv::THRESH_BINARY);   // 大于阈值 → 255（白），否则 → 0（黑）
    // [endregion]
    m_fg = cv::countNonZero(bin);
    m_binary = QImage(bin.data, bin.cols, bin.rows, int(bin.step), QImage::Format_Grayscale8).copy();
    emit changed();
}
