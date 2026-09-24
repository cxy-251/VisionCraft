#pragma once

#include "core/IToolPage.h"
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <chrono>
#include <opencv2/core.hpp>

class LiveScreenPipelinePage : public IToolPage {
    Q_OBJECT
public:
    explicit LiveScreenPipelinePage(QWidget *parent = nullptr);
    ~LiveScreenPipelinePage() override = default;

    QString id() const override { return "live_pipeline"; }
    QString title() const override { return "实时屏幕流·算子流水线质检台"; }
    QString description() const override {
        return "以当前屏幕为虚拟超高清摄像机，实时运行 30FPS 算法流水线与工业质检判定。";
    }
    QString icon() const override { return "⚡"; }

    void onActivated() override;
    void onDeactivated() override;

private slots:
    void toggleStreaming();
    void processNextFrame();
    void clearHistoryTable();

private:
    void setupUI();
    void initPipelineControls(QWidget *parent, QVBoxLayout *layout);
    void setupTableStyle();
    void addInspectionRecord(int frameNum, int objCount, double maxArea, bool passed, double latencyMs);

    // 采集定时器与性能监测
    QTimer *m_streamTimer = nullptr;
    bool m_isStreaming = false;
    int m_frameCount = 0;
    std::chrono::steady_clock::time_point m_lastFpsCheck;
    int m_fpsCounter = 0;
    double m_currentFps = 0.0;

    // 控制面板控件
    QPushButton *m_streamToggleBtn = nullptr;
    QComboBox *m_captureModeCombo = nullptr;
    QComboBox *m_fpsCombo = nullptr;
    QLabel *m_statusMetricsLabel = nullptr;

    // 算子流水线配置
    // Step 1: 预处理与色彩变换
    QCheckBox *m_chkBlur = nullptr;
    QSlider *m_blurKernelSlider = nullptr;
    QLabel *m_blurKernelValLabel = nullptr;
    QCheckBox *m_chkBilateral = nullptr;

    // Step 2: 阈值二值化与边缘提取
    QCheckBox *m_chkCanny = nullptr;
    QSlider *m_cannyLowSlider = nullptr;
    QLabel *m_cannyLowValLabel = nullptr;
    QSlider *m_cannyHighSlider = nullptr;
    QLabel *m_cannyHighValLabel = nullptr;
    QCheckBox *m_chkAdaptiveThresh = nullptr;

    // Step 3: 形态学提纯
    QCheckBox *m_chkMorph = nullptr;
    QComboBox *m_morphTypeCombo = nullptr;
    QSlider *m_morphKernelSlider = nullptr;

    // Step 4: 工业质检与目标分析 (Overlay)
    QCheckBox *m_chkContours = nullptr;
    QSlider *m_minAreaSlider = nullptr;
    QLabel *m_minAreaValLabel = nullptr;
    QCheckBox *m_chkOrbKeypoints = nullptr;
    QCheckBox *m_chkHarrisCorners = nullptr;

    // 画廊与日志
    QLabel *m_liveInputLabel = nullptr;
    QLabel *m_liveOutputLabel = nullptr;
    QLabel *m_livePassBadgeLabel = nullptr;
    QTableWidget *m_logTable = nullptr;
};
