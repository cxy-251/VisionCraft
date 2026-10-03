#pragma once

#include <QImage>
#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <opencv2/core.hpp>

// 「全局阈值与 OTSU」的演示：一张工件图、它的灰度直方图、可拖动的阈值，以及大津法自动算出的阈值
class ThresholdDemo : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold NOTIFY changed)
    Q_PROPERTY(int otsu READ otsu NOTIFY partChanged)
    Q_PROPERTY(double lighting READ lighting WRITE setLighting NOTIFY partChanged)   // 光照不均的程度 0~1
    Q_PROPERTY(QImage source READ source NOTIFY partChanged)
    Q_PROPERTY(QImage binary READ binary NOTIFY changed)
    Q_PROPERTY(QVariantList histogram READ histogram NOTIFY partChanged)   // 256 个计数
    Q_PROPERTY(int foregroundPixels READ foregroundPixels NOTIFY changed)

public:
    explicit ThresholdDemo(QObject *parent = nullptr);

    int threshold() const { return m_threshold; }
    void setThreshold(int t);
    int otsu() const { return m_otsu; }
    double lighting() const { return m_lighting; }
    void setLighting(double l);
    QImage source() const { return m_source; }
    QImage binary() const { return m_binary; }
    QVariantList histogram() const { return m_hist; }
    int foregroundPixels() const { return m_fg; }

    Q_INVOKABLE void newPart();

    // 给灰度图加上不均匀的光照（lighting 0~1），演示和示例程序共用
    static cv::Mat applyLighting(const cv::Mat &gray, double lighting);
    Q_INVOKABLE void useOtsu() { setThreshold(m_otsu); }

signals:
    void changed();
    void partChanged();

private:
    void analyse();
    void apply();

    cv::Mat m_partGray;   // 原始工件（灰度）
    cv::Mat m_gray;       // 加上光照不均后的
    QImage m_source, m_binary;
    QVariantList m_hist;
    int m_threshold = 128;
    int m_otsu = 0;
    int m_fg = 0;
    double m_lighting = 0.0;
    unsigned m_seed = 7;
};
