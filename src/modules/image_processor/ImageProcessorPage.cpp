#include "ImageProcessorPage.h"
#include "core/ScreenCapture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QScrollArea>
#include <QFrame>
#include <QMessageBox>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

ImageProcessorPage::ImageProcessorPage(QWidget *parent) : IToolPage(parent) {
    setupUI();
}

void ImageProcessorPage::setupUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(20);

    // 左侧面板
    auto *leftPanel = new QFrame(this);
    leftPanel->setFixedWidth(320);
    leftPanel->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(14);

    auto *title = new QLabel("🛠️ 图像算法与滤镜参数", leftPanel);
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a; border: none;");
    leftLayout->addWidget(title);

    // 输入源选择
    auto *inputBtnRow = new QHBoxLayout();
    auto *openFileBtn = new QPushButton("📂 打开图片", leftPanel);
    openFileBtn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 7px; font-weight: 500; }"
                               "QPushButton:hover { background-color: #e2e8f0; }");
    connect(openFileBtn, &QPushButton::clicked, this, &ImageProcessorPage::openImageFile);

    auto *grabScreenBtn = new QPushButton("📸 截取屏幕", leftPanel);
    grabScreenBtn->setStyleSheet("QPushButton { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 7px; font-weight: 500; }"
                                 "QPushButton:hover { background-color: #e2e8f0; }");
    connect(grabScreenBtn, &QPushButton::clicked, this, &ImageProcessorPage::captureScreenAsInput);

    inputBtnRow->addWidget(openFileBtn);
    inputBtnRow->addWidget(grabScreenBtn);
    leftLayout->addLayout(inputBtnRow);

    // 算法模式选择
    auto *modeLabel = new QLabel("当前算法模式:", leftPanel);
    modeLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #475569; border: none;");
    leftLayout->addWidget(modeLabel);

    m_modeCombo = new QComboBox(leftPanel);
    m_modeCombo->addItem("Canny 边缘检测 (Edge Detection)", "canny");
    m_modeCombo->addItem("高斯滤波模糊 (Gaussian Blur)", "blur");
    m_modeCombo->addItem("自适应二值化 (Thresholding)", "threshold");
    m_modeCombo->addItem("灰度化 (Grayscale)", "gray");
    m_modeCombo->setStyleSheet("QComboBox { padding: 6px; border: 1px solid #cbd5e1; border-radius: 6px; }");
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageProcessorPage::applyProcessing);
    leftLayout->addWidget(m_modeCombo);

    // 参数 1 滑条
    auto *p1Row = new QHBoxLayout();
    m_param1NameLabel = new QLabel("低阈值 (Threshold 1):", leftPanel);
    m_param1NameLabel->setStyleSheet("font-size: 12px; color: #475569; border: none;");
    m_param1ValLabel = new QLabel("50", leftPanel);
    m_param1ValLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #2563eb; border: none;");
    p1Row->addWidget(m_param1NameLabel);
    p1Row->addStretch();
    p1Row->addWidget(m_param1ValLabel);
    leftLayout->addLayout(p1Row);

    m_param1Slider = new QSlider(Qt::Horizontal, leftPanel);
    m_param1Slider->setRange(1, 255);
    m_param1Slider->setValue(50);
    connect(m_param1Slider, &QSlider::valueChanged, this, [this](int val) {
        m_param1ValLabel->setText(QString::number(val));
        applyProcessing();
    });
    leftLayout->addWidget(m_param1Slider);

    // 参数 2 滑条
    auto *p2Row = new QHBoxLayout();
    m_param2NameLabel = new QLabel("高阈值 (Threshold 2):", leftPanel);
    m_param2NameLabel->setStyleSheet("font-size: 12px; color: #475569; border: none;");
    m_param2ValLabel = new QLabel("150", leftPanel);
    m_param2ValLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #2563eb; border: none;");
    p2Row->addWidget(m_param2NameLabel);
    p2Row->addStretch();
    p2Row->addWidget(m_param2ValLabel);
    leftLayout->addLayout(p2Row);

    m_param2Slider = new QSlider(Qt::Horizontal, leftPanel);
    m_param2Slider->setRange(1, 255);
    m_param2Slider->setValue(150);
    connect(m_param2Slider, &QSlider::valueChanged, this, [this](int val) {
        m_param2ValLabel->setText(QString::number(val));
        applyProcessing();
    });
    leftLayout->addWidget(m_param2Slider);

    // 保存按钮
    auto *saveBtn = new QPushButton("💾 导出处理后图片...", leftPanel);
    saveBtn->setStyleSheet("QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: 600; }"
                           "QPushButton:hover { background-color: #1d4ed8; }");
    connect(saveBtn, &QPushButton::clicked, this, &ImageProcessorPage::saveResultImage);
    leftLayout->addWidget(saveBtn);

    m_infoLabel = new QLabel("就绪。载入图片并调节滑块可实时查看算法效果。", leftPanel);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("background-color: #f8fafc; border-radius: 6px; padding: 10px; font-size: 12px; color: #64748b; border: none;");
    leftLayout->addWidget(m_infoLabel);

    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel);

    // 右侧大图画廊
    auto *rightPanel = new QFrame(this);
    rightPanel->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(15, 15, 15, 15);

    auto *scrollArea = new QScrollArea(rightPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background: #0f172a; border-radius: 8px; border: none;");

    m_displayLabel = new QLabel(scrollArea);
    m_displayLabel->setAlignment(Qt::AlignCenter);
    m_displayLabel->setStyleSheet("color: #64748b; font-size: 14px;");
    m_displayLabel->setText("请载入图像以进行实时视觉处理");
    scrollArea->setWidget(m_displayLabel);
    rightLayout->addWidget(scrollArea, 1);

    mainLayout->addWidget(rightPanel, 1);
}

void ImageProcessorPage::onActivated() {
    if (m_originalMat.empty()) {
        captureScreenAsInput();
    }
}

void ImageProcessorPage::captureScreenAsInput() {
    QImage img = ScreenCapture::grabScreen(0);
    if (!img.isNull()) {
        m_originalMat = ScreenCapture::qImageToMat(img);
        applyProcessing();
    }
}

void ImageProcessorPage::openImageFile() {
    QString path = QFileDialog::getOpenFileName(this, "打开待处理图像", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    cv::Mat img = cv::imread(path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
    if (img.empty()) {
        QMessageBox::warning(this, "错误", "无法解析该图像文件！");
        return;
    }
    m_originalMat = img;
    applyProcessing();
}

void ImageProcessorPage::applyProcessing() {
    if (m_originalMat.empty()) return;

    QString mode = m_modeCombo->currentData().toString();
    int p1 = m_param1Slider->value();
    int p2 = m_param2Slider->value();

    if (mode == "canny") {
        m_param1NameLabel->setText("低阈值 (Low):");
        m_param2NameLabel->setText("高阈值 (High):");
        m_param1Slider->setEnabled(true);
        m_param2Slider->setEnabled(true);

        cv::Mat gray;
        cv::cvtColor(m_originalMat, gray, cv::COLOR_BGR2GRAY);
        cv::Canny(gray, m_processedMat, p1, p2);
        m_infoLabel->setText(QString("Canny 算子：提取图像梯度边缘 (低=%1, 高=%2)").arg(p1).arg(p2));
    } 
    else if (mode == "blur") {
        m_param1NameLabel->setText("卷积核半径 (Kernel):");
        m_param2NameLabel->setText("未启用:");
        m_param1Slider->setEnabled(true);
        m_param2Slider->setEnabled(false);

        int ksize = (p1 / 10) * 2 + 1; // 必须是奇数
        cv::GaussianBlur(m_originalMat, m_processedMat, cv::Size(ksize, ksize), 0);
        m_infoLabel->setText(QString("高斯模糊：卷积核大小 %1 x %1").arg(ksize));
    }
    else if (mode == "threshold") {
        m_param1NameLabel->setText("分割阈值 (Threshold):");
        m_param2NameLabel->setText("未启用:");
        m_param1Slider->setEnabled(true);
        m_param2Slider->setEnabled(false);

        cv::Mat gray;
        cv::cvtColor(m_originalMat, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, m_processedMat, p1, 255, cv::THRESH_BINARY);
        m_infoLabel->setText(QString("二值化分割：以灰度值 %1 为界将像素二分为黑白").arg(p1));
    }
    else if (mode == "gray") {
        m_param1Slider->setEnabled(false);
        m_param2Slider->setEnabled(false);
        cv::cvtColor(m_originalMat, m_processedMat, cv::COLOR_BGR2GRAY);
        m_infoLabel->setText("灰度化：单通道亮度映射");
    }

    // 显示到界面
    QImage qImg = ScreenCapture::matToQImage(m_processedMat);
    m_displayLabel->setPixmap(QPixmap::fromImage(qImg).scaled(
        m_displayLabel->parentWidget()->size() - QSize(20, 20),
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    ));
}

void ImageProcessorPage::saveResultImage() {
    if (m_processedMat.empty()) return;

    QString savePath = QFileDialog::getSaveFileName(this, "保存处理后图像", "processed_result.png", "PNG (*.png);;JPG (*.jpg)");
    if (savePath.isEmpty()) return;

    cv::imwrite(savePath.toLocal8Bit().constData(), m_processedMat);
    QMessageBox::information(this, "成功", "图片已成功导出至：\n" + savePath);
}
