#include "VisionMatcherPage.h"
#include "ThemeManager.h"
#include "core/ScreenCapture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <chrono>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

VisionMatcherPage::VisionMatcherPage(QWidget *parent) : IToolPage(parent) {
    setupUI();
}

void VisionMatcherPage::setupUI() {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(25, 20, 25, 20);
    mainLayout->setSpacing(20);

    // 左侧：控制面板与小样预览 (固定宽度 330)
    auto *leftPanel = new QFrame(this);
    leftPanel->setObjectName("PanelCard");
    leftPanel->setFixedWidth(330);

    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(18, 18, 18, 18);
    leftLayout->setSpacing(14);

    auto *panelTitle = new QLabel("⚙️ 自动化匹配控制台", leftPanel);
    panelTitle->setObjectName("CardTitle");
    leftLayout->addWidget(panelTitle);

    // 按钮 1：截取当前屏幕
    m_captureBtn = new QPushButton("📸 1. 抓取全屏图像", leftPanel);
    m_captureBtn->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: 600; font-size: 13px; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    connect(m_captureBtn, &QPushButton::clicked, this, &VisionMatcherPage::captureCurrentScreen);
    leftLayout->addWidget(m_captureBtn);

    // 按钮 2：选区/设置小样
    auto *sampleBtn = new QPushButton("🎯 2. 截取屏幕中央为小样", leftPanel);
    sampleBtn->setStyleSheet(
        "QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 6px; padding: 9px; font-weight: 500; font-size: 13px; }"
        "QPushButton:hover { background-color: #0f766e; }"
    );
    connect(sampleBtn, &QPushButton::clicked, this, &VisionMatcherPage::setSampleTemplate);
    leftLayout->addWidget(sampleBtn);

    auto *loadCustomBtn = new QPushButton("📂 或加载本地小样图片...", leftPanel);
    loadCustomBtn->setObjectName("SecondaryBtn");
    connect(loadCustomBtn, &QPushButton::clicked, this, &VisionMatcherPage::loadCustomTemplate);
    leftLayout->addWidget(loadCustomBtn);

    // 小样预览区域
    auto *tplLabel = new QLabel("当前目标小样 (Template):", leftPanel);
    tplLabel->setObjectName("CardSubTitle");
    leftLayout->addWidget(tplLabel);

    m_templatePreviewLabel = new QLabel("暂无小样", leftPanel);
    m_templatePreviewLabel->setAlignment(Qt::AlignCenter);
    m_templatePreviewLabel->setFixedHeight(90);
    auto updatePreviewStyle = [this](bool isDark) {
        if (isDark) {
            m_templatePreviewLabel->setStyleSheet("background-color: #0f172a; border: 1px dashed #334155; border-radius: 6px; color: #64748b; font-size: 12px;");
        } else {
            m_templatePreviewLabel->setStyleSheet("background-color: #f8fafc; border: 1px dashed #cbd5e1; border-radius: 6px; color: #94a3b8; font-size: 12px;");
        }
    };
    updatePreviewStyle(ThemeManager::instance().isDarkMode());
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, updatePreviewStyle);
    leftLayout->addWidget(m_templatePreviewLabel);

    // 阈值滑块
    auto *threshRow = new QHBoxLayout();
    auto *threshTitle = new QLabel("匹配置信度阈值:", leftPanel);
    threshTitle->setObjectName("CardSubTitle");
    m_thresholdValueLabel = new QLabel("85%", leftPanel);
    m_thresholdValueLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #38bdf8; border: none;");
    threshRow->addWidget(threshTitle);
    threshRow->addStretch();
    threshRow->addWidget(m_thresholdValueLabel);
    leftLayout->addLayout(threshRow);

    m_thresholdSlider = new QSlider(Qt::Horizontal, leftPanel);
    m_thresholdSlider->setRange(50, 99);
    m_thresholdSlider->setValue(85);
    connect(m_thresholdSlider, &QSlider::valueChanged, this, [this](int val) {
        m_thresholdValueLabel->setText(QString("%1%").arg(val));
    });
    leftLayout->addWidget(m_thresholdSlider);

    // 按钮 3：执行模板匹配
    m_runMatchBtn = new QPushButton("⚡ 3. 运行毫秒级找图定位", leftPanel);
    m_runMatchBtn->setStyleSheet(
        "QPushButton { background-color: #16a34a; color: white; border: none; border-radius: 6px; padding: 12px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #15803d; }"
    );
    connect(m_runMatchBtn, &QPushButton::clicked, this, &VisionMatcherPage::runTemplateMatching);
    leftLayout->addWidget(m_runMatchBtn);

    // 统计结果文本框
    m_statsLabel = new QLabel("就绪。请先抓取屏幕，设置小样后点击找图。", leftPanel);
    m_statsLabel->setObjectName("StatusBox");
    m_statsLabel->setWordWrap(true);
    leftLayout->addWidget(m_statsLabel);

    leftLayout->addStretch();
    mainLayout->addWidget(leftPanel);

    // 右侧：大图实时结果画廊
    auto *rightPanel = new QFrame(this);
    rightPanel->setObjectName("PanelCard");
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(15, 15, 15, 15);

    auto *rightHeader = new QHBoxLayout();
    auto *rightTitle = new QLabel("🖥️ 屏幕与匹配标注视图", rightPanel);
    rightTitle->setObjectName("CardTitle");
    rightHeader->addWidget(rightTitle);
    rightHeader->addStretch();
    rightLayout->addLayout(rightHeader);

    auto *scrollArea = new QScrollArea(rightPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("background: #0f172a; border-radius: 8px; border: none;");

    m_previewLabel = new QLabel(scrollArea);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("color: #64748b; font-size: 14px;");
    m_previewLabel->setText("点击左侧「1. 抓取全屏图像」载入当前屏幕画面");
    scrollArea->setWidget(m_previewLabel);
    rightLayout->addWidget(scrollArea, 1);

    mainLayout->addWidget(rightPanel, 1);
}

void VisionMatcherPage::onActivated() {
    if (m_screenMat.empty()) {
        captureCurrentScreen();
    }
}

void VisionMatcherPage::captureCurrentScreen() {
    QImage screenImg = ScreenCapture::grabScreen(0);
    if (screenImg.isNull()) {
        m_statsLabel->setText("❌ 屏幕抓取失败！");
        return;
    }
    m_screenMat = ScreenCapture::qImageToMat(screenImg);
    m_resultDisplayMat = m_screenMat.clone();
    updateResultDisplay();
    m_statsLabel->setText(QString("✅ 成功捕获屏幕：%1 x %2 像素").arg(m_screenMat.cols).arg(m_screenMat.rows));
}

void VisionMatcherPage::setSampleTemplate() {
    if (m_screenMat.empty()) {
        captureCurrentScreen();
    }
    if (m_screenMat.empty()) return;

    // 从屏幕中央裁剪一个 160x70 区域作为演示模板
    int cropW = std::min(160, m_screenMat.cols / 4);
    int cropH = std::min(70, m_screenMat.rows / 4);
    int startX = (m_screenMat.cols - cropW) / 2;
    int startY = (m_screenMat.rows - cropH) / 2;

    cv::Rect roi(startX, startY, cropW, cropH);
    m_templateMat = m_screenMat(roi).clone();

    // 更新左侧小样预览
    QImage tplImg = ScreenCapture::matToQImage(m_templateMat);
    m_templatePreviewLabel->setPixmap(QPixmap::fromImage(tplImg).scaled(
        m_templatePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
    ));
    m_statsLabel->setText(QString("🎯 小样已就绪 (尺寸: %1x%2)。现在点击「3. 运行毫秒级找图」验证。")
                          .arg(cropW).arg(cropH));
}

void VisionMatcherPage::loadCustomTemplate() {
    QString path = QFileDialog::getOpenFileName(this, "选择小样图片", "", "Images (*.png *.jpg *.bmp)");
    if (path.isEmpty()) return;

    cv::Mat loaded = cv::imread(path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
    if (loaded.empty()) {
        QMessageBox::warning(this, "错误", "无法读取选择的图片！");
        return;
    }
    m_templateMat = loaded;
    QImage tplImg = ScreenCapture::matToQImage(m_templateMat);
    m_templatePreviewLabel->setPixmap(QPixmap::fromImage(tplImg).scaled(
        m_templatePreviewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation
    ));
    m_statsLabel->setText(QString("🎯 外部小样已载入 (%1x%2)").arg(m_templateMat.cols).arg(m_templateMat.rows));
}

void VisionMatcherPage::runTemplateMatching() {
    if (m_screenMat.empty() || m_templateMat.empty()) {
        m_statsLabel->setText("⚠️ 请先确保已抓取屏幕且设置了小样！");
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // 核心算法：归一化互相关匹配 (Normalized Cross-Correlation)
    cv::Mat result;
    cv::matchTemplate(m_screenMat, m_templateMat, result, cv::TM_CCOEFF_NORMED);

    double minVal = 0.0, maxVal = 0.0;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    auto end = std::chrono::high_resolution_clock::now();
    double durationMs = std::chrono::duration<double, std::milli>(end - start).count();

    double threshold = m_thresholdSlider->value() / 100.0;

    m_resultDisplayMat = m_screenMat.clone();

    if (maxVal >= threshold) {
        // 匹配成功，绘制高亮绿框与十字准星
        cv::Rect matchRect(maxLoc.x, maxLoc.y, m_templateMat.cols, m_templateMat.rows);
        cv::rectangle(m_resultDisplayMat, matchRect, cv::Scalar(0, 255, 0), 3, cv::LINE_AA);

        // 绘制半透明标签背景
        std::string scoreText = "Match: " + std::to_string(int(maxVal * 1000) / 10.0) + "%";
        cv::putText(m_resultDisplayMat, scoreText, cv::Point(maxLoc.x, std::max(25, maxLoc.y - 8)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2, cv::LINE_AA);

        m_statsLabel->setText(
            QString("🎉 <b>找图成功！</b><br>"
                    "置信度: <font color='#16a34a'><b>%1%</b></font><br>"
                    "坐标位置: X=%2, Y=%3<br>"
                    "小样尺寸: %4 x %5<br>"
                    "匹配算法耗时: <b>%6 ms</b>")
            .arg(QString::number(maxVal * 100, 'f', 1))
            .arg(maxLoc.x).arg(maxLoc.y)
            .arg(m_templateMat.cols).arg(m_templateMat.rows)
            .arg(QString::number(durationMs, 'f', 2))
        );
    } else {
        // 未达到阈值
        m_statsLabel->setText(
            QString("❌ <b>未找到目标</b><br>"
                    "当前最高相似度: %1% (低于设定阈值 %2%)<br>"
                    "匹配耗时: %3 ms")
            .arg(QString::number(maxVal * 100, 'f', 1))
            .arg(m_thresholdSlider->value())
            .arg(QString::number(durationMs, 'f', 2))
        );
    }

    updateResultDisplay();
}

void VisionMatcherPage::updateResultDisplay() {
    if (m_resultDisplayMat.empty()) return;
    QImage qImg = ScreenCapture::matToQImage(m_resultDisplayMat);
    // 缩放到画布可显示大小
    m_previewLabel->setPixmap(QPixmap::fromImage(qImg).scaled(
        m_previewLabel->parentWidget()->size() - QSize(20, 20),
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    ));
}
