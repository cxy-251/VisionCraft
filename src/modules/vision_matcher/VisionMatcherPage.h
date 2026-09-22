#pragma once

#include "core/IToolPage.h"
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QScrollArea>
#include <opencv2/core.hpp>

class VisionMatcherPage : public IToolPage {
    Q_OBJECT
public:
    explicit VisionMatcherPage(QWidget *parent = nullptr);
    ~VisionMatcherPage() override = default;

    QString id() const override { return "vision_matcher"; }
    QString title() const override { return "屏幕找图与模板匹配引擎"; }
    QString description() const override { 
        return "游戏辅助与桌面自动化核心：利用 OpenCV 归一化互相关算法，毫秒级定位屏幕上的按钮与图标小样。"; 
    }
    QString icon() const override { return "🎯"; }

    void onActivated() override;

private slots:
    void captureCurrentScreen();
    void setSampleTemplate();
    void runTemplateMatching();
    void loadCustomTemplate();

private:
    void setupUI();
    void updateResultDisplay();

    // 图像数据
    cv::Mat m_screenMat;
    cv::Mat m_templateMat;
    cv::Mat m_resultDisplayMat;

    // UI 控件
    QLabel *m_previewLabel = nullptr;
    QLabel *m_templatePreviewLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
    QSlider *m_thresholdSlider = nullptr;
    QLabel *m_thresholdValueLabel = nullptr;
    QPushButton *m_captureBtn = nullptr;
    QPushButton *m_runMatchBtn = nullptr;
};
