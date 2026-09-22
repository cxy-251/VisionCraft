#pragma once

#include "core/IToolPage.h"
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <opencv2/core.hpp>

class ImageProcessorPage : public IToolPage {
    Q_OBJECT
public:
    explicit ImageProcessorPage(QWidget *parent = nullptr);
    ~ImageProcessorPage() override = default;

    QString id() const override { return "image_processor"; }
    QString title() const override { return "OpenCV 交互式图像工坊"; }
    QString description() const override { 
        return "直观体验计算机视觉底层算法：高斯滤波、Canny 边缘检测、二值化与色彩通道实时交互。"; 
    }
    QString icon() const override { return "🎨"; }

    void onActivated() override;

private slots:
    void openImageFile();
    void captureScreenAsInput();
    void applyProcessing();
    void saveResultImage();

private:
    void setupUI();

    cv::Mat m_originalMat;
    cv::Mat m_processedMat;

    QLabel *m_displayLabel = nullptr;
    QComboBox *m_modeCombo = nullptr;
    QSlider *m_param1Slider = nullptr;
    QSlider *m_param2Slider = nullptr;
    QLabel *m_param1NameLabel = nullptr;
    QLabel *m_param2NameLabel = nullptr;
    QLabel *m_param1ValLabel = nullptr;
    QLabel *m_param2ValLabel = nullptr;
    QLabel *m_infoLabel = nullptr;
};
