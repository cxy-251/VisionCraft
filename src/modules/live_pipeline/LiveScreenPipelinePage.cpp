#include "LiveScreenPipelinePage.h"
#include "ThemeManager.h"
#include "core/ScreenCapture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QHeaderView>
#include <QGuiApplication>
#include <QScreen>
#include <QCursor>
#include <QDateTime>
#include <fmt/format.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

LiveScreenPipelinePage::LiveScreenPipelinePage(QWidget *parent) : IToolPage(parent) {
    setupUI();
}

void LiveScreenPipelinePage::setupUI() {
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 12, 15, 12);
    rootLayout->setSpacing(14);

    // ========================================================
    // 左侧：流水线控制器 (固定宽度 340)
    // ========================================================
    auto *leftScroll = new QScrollArea(this);
    leftScroll->setWidgetResizable(true);
    leftScroll->setFixedWidth(340);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setStyleSheet("background: transparent; border: none;");

    auto *leftPanel = new QFrame(leftScroll);
    leftPanel->setObjectName("PanelCard");
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(16, 16, 16, 16);
    leftLayout->setSpacing(12);

    auto *panelTitle = new QLabel("⚡ 实时视觉流与流水线编排", leftPanel);
    panelTitle->setObjectName("CardTitle");
    leftLayout->addWidget(panelTitle);

    // 1. 虚拟相机启动器
    m_streamToggleBtn = new QPushButton("▶️ 启动实时屏幕视觉流", leftPanel);
    m_streamToggleBtn->setStyleSheet(
        "QPushButton { background-color: #16a34a; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #15803d; }"
    );
    connect(m_streamToggleBtn, &QPushButton::clicked, this, &LiveScreenPipelinePage::toggleStreaming);
    leftLayout->addWidget(m_streamToggleBtn);

    // 2. 采集模式与帧率
    auto *modeLabel = new QLabel("相机输入源采集模式:", leftPanel);
    modeLabel->setObjectName("CardSubTitle");
    leftLayout->addWidget(modeLabel);

    m_captureModeCombo = new QComboBox(leftPanel);
    m_captureModeCombo->addItem("🔍 鼠标跟随动态放大 (Microscope 640x400)", "mouse");
    m_captureModeCombo->addItem("🪟 屏幕中心工作区 (Center 800x500)", "center");
    m_captureModeCombo->addItem("🖥️ 桌面全屏自适应 (Full Desktop)", "full");
    leftLayout->addWidget(m_captureModeCombo);

    auto *fpsRow = new QHBoxLayout();
    auto *fpsLabel = new QLabel("目标帧率 (FPS):", leftPanel);
    fpsLabel->setObjectName("CardSubTitle");
    m_fpsCombo = new QComboBox(leftPanel);
    m_fpsCombo->addItem("30 FPS (推荐流式)", 33);
    m_fpsCombo->addItem("60 FPS (高帧极速)", 16);
    m_fpsCombo->addItem("15 FPS (低功耗)", 66);
    connect(m_fpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int interval = m_fpsCombo->currentData().toInt();
        if (m_streamTimer && m_isStreaming) {
            m_streamTimer->start(interval);
        }
    });
    fpsRow->addWidget(fpsLabel);
    fpsRow->addWidget(m_fpsCombo);
    leftLayout->addLayout(fpsRow);

    // 3. 实时诊断指标框
    m_statusMetricsLabel = new QLabel("⏸️ 视觉流已暂停。点击上方绿色按钮启动。", leftPanel);
    m_statusMetricsLabel->setObjectName("StatusBox");
    m_statusMetricsLabel->setWordWrap(true);
    leftLayout->addWidget(m_statusMetricsLabel);

    // 4. 算子流水线配置卡片
    initPipelineControls(leftPanel, leftLayout);

    leftLayout->addStretch();
    leftScroll->setWidget(leftPanel);
    rootLayout->addWidget(leftScroll);

    // ========================================================
    // 右侧：实时视窗画廊 + 工业质检动态数据表
    // ========================================================
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    // 顶部视窗卡片
    auto *viewportCard = new QFrame(rightWidget);
    viewportCard->setObjectName("PanelCard");
    auto *vpLayout = new QVBoxLayout(viewportCard);
    vpLayout->setContentsMargins(14, 12, 14, 12);
    vpLayout->setSpacing(10);

    auto *vpHeader = new QHBoxLayout();
    auto *vpTitle = new QLabel("🖥️ 双路实时视觉流对比视窗", viewportCard);
    vpTitle->setObjectName("CardTitle");
    vpHeader->addWidget(vpTitle);

    vpHeader->addStretch();

    m_livePassBadgeLabel = new QLabel("✅ 质检状态: 就绪", viewportCard);
    m_livePassBadgeLabel->setStyleSheet("background-color: #0284c7; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; font-size: 12px;");
    vpHeader->addWidget(m_livePassBadgeLabel);
    vpLayout->addLayout(vpHeader);

    // 双通道视窗
    auto *viewRow = new QHBoxLayout();
    viewRow->setSpacing(10);

    auto *leftBox = new QFrame(viewportCard);
    leftBox->setStyleSheet("background-color: #0f172a; border-radius: 6px; border: 1px solid #334155;");
    auto *leftBoxLay = new QVBoxLayout(leftBox);
    leftBoxLay->setContentsMargins(8, 8, 8, 8);
    auto *leftTag = new QLabel("1️⃣ 虚拟相机原始屏幕流", leftBox);
    leftTag->setStyleSheet("color: #94a3b8; font-size: 11px; font-weight: bold;");
    m_liveInputLabel = new QLabel("等待流启动...", leftBox);
    m_liveInputLabel->setAlignment(Qt::AlignCenter);
    m_liveInputLabel->setStyleSheet("color: #64748b; font-size: 13px;");
    leftBoxLay->addWidget(leftTag);
    leftBoxLay->addWidget(m_liveInputLabel, 1);

    auto *rightBox = new QFrame(viewportCard);
    rightBox->setStyleSheet("background-color: #0f172a; border-radius: 6px; border: 1px solid #334155;");
    auto *rightBoxLay = new QVBoxLayout(rightBox);
    rightBoxLay->setContentsMargins(8, 8, 8, 8);
    auto *rightTag = new QLabel("2️⃣ 算法流水线实时分析与标注", rightBox);
    rightTag->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold;");
    m_liveOutputLabel = new QLabel("等待流启动...", rightBox);
    m_liveOutputLabel->setAlignment(Qt::AlignCenter);
    m_liveOutputLabel->setStyleSheet("color: #64748b; font-size: 13px;");
    rightBoxLay->addWidget(rightTag);
    rightBoxLay->addWidget(m_liveOutputLabel, 1);

    viewRow->addWidget(leftBox, 1);
    viewRow->addWidget(rightBox, 1);
    vpLayout->addLayout(viewRow, 1);
    rightLayout->addWidget(viewportCard, 3);

    // 底部表格卡片
    auto *tableCard = new QFrame(rightWidget);
    tableCard->setObjectName("PanelCard");
    auto *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(14, 12, 14, 12);
    tableLayout->setSpacing(8);

    auto *tblHeader = new QHBoxLayout();
    auto *tblTitle = new QLabel("📊 实时质检结果监测流水表 (Model/View 架构展现)", tableCard);
    tblTitle->setObjectName("CardSubTitle");
    tblTitle->setStyleSheet("font-weight: 700; font-size: 14px;");
    tblHeader->addWidget(tblTitle);
    tblHeader->addStretch();

    auto *clearBtn = new QPushButton("🗑️ 清空记录", tableCard);
    clearBtn->setObjectName("SecondaryBtn");
    connect(clearBtn, &QPushButton::clicked, this, &LiveScreenPipelinePage::clearHistoryTable);
    tblHeader->addWidget(clearBtn);
    tableLayout->addLayout(tblHeader);

    m_logTable = new QTableWidget(tableCard);
    m_logTable->setColumnCount(6);
    m_logTable->setHorizontalHeaderLabels({"帧编号", "采样时间戳", "检出目标数", "最大目标面积", "质检判定", "单帧算法延迟"});
    m_logTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setMinimumHeight(160);
    setupTableStyle();
    tableLayout->addWidget(m_logTable, 1);

    rightLayout->addWidget(tableCard, 2);
    rootLayout->addWidget(rightWidget, 1);

    // 初始化定时器
    m_streamTimer = new QTimer(this);
    connect(m_streamTimer, &QTimer::timeout, this, &LiveScreenPipelinePage::processNextFrame);

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this](bool) {
        setupTableStyle();
    });
}

void LiveScreenPipelinePage::initPipelineControls(QWidget *parent, QVBoxLayout *layout) {
    // 阶段 1: 预处理
    auto *sep1 = new QLabel("── 1. 预处理算子 (Preprocessing) ──", parent);
    sep1->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold; margin-top: 6px;");
    layout->addWidget(sep1);

    m_chkBlur = new QCheckBox("高斯滤波降噪 (Gaussian Blur)", parent);
    m_chkBlur->setChecked(true);
    layout->addWidget(m_chkBlur);

    auto *blurRow = new QHBoxLayout();
    auto *blurTitle = new QLabel("核尺寸:", parent);
    blurTitle->setStyleSheet("font-size: 11px; color: #94a3b8;");
    m_blurKernelValLabel = new QLabel("5", parent);
    m_blurKernelValLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #38bdf8;");
    m_blurKernelSlider = new QSlider(Qt::Horizontal, parent);
    m_blurKernelSlider->setRange(1, 15);
    m_blurKernelSlider->setSingleStep(2);
    m_blurKernelSlider->setValue(5);
    connect(m_blurKernelSlider, &QSlider::valueChanged, this, [this](int v) {
        if (v % 2 == 0) v += 1;
        m_blurKernelValLabel->setText(QString::number(v));
    });
    blurRow->addWidget(blurTitle);
    blurRow->addWidget(m_blurKernelSlider);
    blurRow->addWidget(m_blurKernelValLabel);
    layout->addLayout(blurRow);

    m_chkBilateral = new QCheckBox("双边保边滤波 (Bilateral)", parent);
    layout->addWidget(m_chkBilateral);

    // 阶段 2: 阈值与边缘
    auto *sep2 = new QLabel("── 2. 边缘与二值化 (Edge/Binary) ──", parent);
    sep2->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold; margin-top: 6px;");
    layout->addWidget(sep2);

    m_chkCanny = new QCheckBox("Canny 边缘提取 (Canny Edges)", parent);
    m_chkCanny->setChecked(true);
    layout->addWidget(m_chkCanny);

    auto *cannyRow = new QHBoxLayout();
    auto *cLowTitle = new QLabel("低阈值:", parent);
    cLowTitle->setStyleSheet("font-size: 11px; color: #94a3b8;");
    m_cannyLowValLabel = new QLabel("60", parent);
    m_cannyLowValLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #38bdf8;");
    m_cannyLowSlider = new QSlider(Qt::Horizontal, parent);
    m_cannyLowSlider->setRange(10, 150);
    m_cannyLowSlider->setValue(60);
    connect(m_cannyLowSlider, &QSlider::valueChanged, this, [this](int v) {
        m_cannyLowValLabel->setText(QString::number(v));
    });
    cannyRow->addWidget(cLowTitle);
    cannyRow->addWidget(m_cannyLowSlider);
    cannyRow->addWidget(m_cannyLowValLabel);
    layout->addLayout(cannyRow);

    auto *cannyHighRow = new QHBoxLayout();
    auto *cHighTitle = new QLabel("高阈值:", parent);
    cHighTitle->setStyleSheet("font-size: 11px; color: #94a3b8;");
    m_cannyHighValLabel = new QLabel("150", parent);
    m_cannyHighValLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #38bdf8;");
    m_cannyHighSlider = new QSlider(Qt::Horizontal, parent);
    m_cannyHighSlider->setRange(50, 255);
    m_cannyHighSlider->setValue(150);
    connect(m_cannyHighSlider, &QSlider::valueChanged, this, [this](int v) {
        m_cannyHighValLabel->setText(QString::number(v));
    });
    cannyHighRow->addWidget(cHighTitle);
    cannyHighRow->addWidget(m_cannyHighSlider);
    cannyHighRow->addWidget(m_cannyHighValLabel);
    layout->addLayout(cannyHighRow);

    m_chkAdaptiveThresh = new QCheckBox("自适应动态二值化 (Adaptive)", parent);
    layout->addWidget(m_chkAdaptiveThresh);

    // 阶段 3: 形态学
    auto *sep3 = new QLabel("── 3. 形态学提纯 (Morphology) ──", parent);
    sep3->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold; margin-top: 6px;");
    layout->addWidget(sep3);

    m_chkMorph = new QCheckBox("形态学滤波过滤", parent);
    layout->addWidget(m_chkMorph);

    auto *morphRow = new QHBoxLayout();
    m_morphTypeCombo = new QComboBox(parent);
    m_morphTypeCombo->addItem("膨胀 (Dilate)", 0);
    m_morphTypeCombo->addItem("腐蚀 (Erode)", 1);
    m_morphTypeCombo->addItem("开运算 (Open-去孤立噪点)", 2);
    m_morphTypeCombo->addItem("闭运算 (Close-缝合孔洞)", 3);
    m_morphKernelSlider = new QSlider(Qt::Horizontal, parent);
    m_morphKernelSlider->setRange(3, 11);
    m_morphKernelSlider->setSingleStep(2);
    m_morphKernelSlider->setValue(3);
    morphRow->addWidget(m_morphTypeCombo, 2);
    morphRow->addWidget(m_morphKernelSlider, 1);
    layout->addLayout(morphRow);

    // 阶段 4: 质检标注与特征追踪
    auto *sep4 = new QLabel("── 4. 工业质检与目标分析 (Overlay) ──", parent);
    sep4->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold; margin-top: 6px;");
    layout->addWidget(sep4);

    m_chkContours = new QCheckBox("轮廓检测与缺陷框选 (Find Contours)", parent);
    m_chkContours->setChecked(true);
    layout->addWidget(m_chkContours);

    auto *areaRow = new QHBoxLayout();
    auto *areaTitle = new QLabel("面积阈值:", parent);
    areaTitle->setStyleSheet("font-size: 11px; color: #94a3b8;");
    m_minAreaValLabel = new QLabel("120 px²", parent);
    m_minAreaValLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #38bdf8;");
    m_minAreaSlider = new QSlider(Qt::Horizontal, parent);
    m_minAreaSlider->setRange(30, 2000);
    m_minAreaSlider->setValue(120);
    connect(m_minAreaSlider, &QSlider::valueChanged, this, [this](int v) {
        m_minAreaValLabel->setText(QString("%1 px²").arg(v));
    });
    areaRow->addWidget(areaTitle);
    areaRow->addWidget(m_minAreaSlider);
    areaRow->addWidget(m_minAreaValLabel);
    layout->addLayout(areaRow);

    m_chkOrbKeypoints = new QCheckBox("ORB 实时特征关键点追踪", parent);
    layout->addWidget(m_chkOrbKeypoints);

    m_chkHarrisCorners = new QCheckBox("Harris 几何拐点红点标注", parent);
    layout->addWidget(m_chkHarrisCorners);
}

void LiveScreenPipelinePage::setupTableStyle() {
    bool isDark = ThemeManager::instance().isDarkMode();
    if (isDark) {
        m_logTable->setStyleSheet(
            "QTableWidget { background-color: #0b1329; color: #f8fafc; border: 1px solid #334155; border-radius: 6px; gridline-color: #1e293b; font-size: 12px; }"
            "QHeaderView::section { background-color: #1e293b; color: #94a3b8; font-weight: 600; border: none; padding: 6px; border-bottom: 1px solid #334155; }"
            "QTableWidget::item:selected { background-color: #2563eb; color: #ffffff; }"
        );
    } else {
        m_logTable->setStyleSheet(
            "QTableWidget { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 6px; gridline-color: #f1f5f9; font-size: 12px; }"
            "QHeaderView::section { background-color: #f8fafc; color: #475569; font-weight: 600; border: none; padding: 6px; border-bottom: 1px solid #cbd5e1; }"
            "QTableWidget::item:selected { background-color: #dbeafe; color: #1e40af; }"
        );
    }
}

void LiveScreenPipelinePage::toggleStreaming() {
    if (m_isStreaming) {
        // 停止流
        m_streamTimer->stop();
        m_isStreaming = false;
        m_streamToggleBtn->setText("▶️ 启动实时屏幕视觉流");
        m_streamToggleBtn->setStyleSheet(
            "QPushButton { background-color: #16a34a; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; }"
            "QPushButton:hover { background-color: #15803d; }"
        );
        m_statusMetricsLabel->setText("⏸️ 视觉流已暂停。");
    } else {
        // 启动流
        int interval = m_fpsCombo->currentData().toInt();
        m_streamTimer->start(interval);
        m_isStreaming = true;
        m_frameCount = 0;
        m_fpsCounter = 0;
        m_lastFpsCheck = std::chrono::steady_clock::now();
        m_streamToggleBtn->setText("⏹️ 暂停实时视觉流");
        m_streamToggleBtn->setStyleSheet(
            "QPushButton { background-color: #dc2626; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; }"
            "QPushButton:hover { background-color: #b91c1c; }"
        );
        m_statusMetricsLabel->setText("🟢 正在捕获实时屏幕流...");
    }
}

void LiveScreenPipelinePage::processNextFrame() {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    QRect screenGeom = screen->geometry();

    QString mode = m_captureModeCombo->currentData().toString();
    QImage rawImg;

    if (mode == "mouse") {
        // 模式 1：跟随鼠标动态显微镜 ROI (640x400)
        QPoint mousePos = QCursor::pos();
        int roiW = 640, roiH = 400;
        int x = qBound(0, mousePos.x() - roiW / 2, qMax(0, screenGeom.width() - roiW));
        int y = qBound(0, mousePos.y() - roiH / 2, qMax(0, screenGeom.height() - roiH));
        rawImg = ScreenCapture::grabScreenRegion(x, y, roiW, roiH);
    } else if (mode == "center") {
        // 模式 2：屏幕中央工作区 (800x500)
        int roiW = 800, roiH = 500;
        int x = qMax(0, (screenGeom.width() - roiW) / 2);
        int y = qMax(0, (screenGeom.height() - roiH) / 2);
        rawImg = ScreenCapture::grabScreenRegion(x, y, roiW, roiH);
    } else {
        // 模式 3：全屏自适应采集
        rawImg = ScreenCapture::grabScreen();
        if (!rawImg.isNull() && (rawImg.width() > 1280 || rawImg.height() > 720)) {
            rawImg = rawImg.scaled(1280, 720, Qt::KeepAspectRatio, Qt::FastTransformation);
        }
    }

    if (rawImg.isNull()) return;

    auto startT = std::chrono::high_resolution_clock::now();

    // 转换成 OpenCV 格式
    cv::Mat inputBgr = ScreenCapture::qImageToMat(rawImg);
    if (inputBgr.empty()) return;

    cv::Mat workMat = inputBgr.clone();

    // ----------------------------------------------------
    // 执行算子流水线
    // ----------------------------------------------------
    // 阶段 1: 预处理
    if (m_chkBlur->isChecked()) {
        int k = m_blurKernelSlider->value();
        if (k % 2 == 0) k += 1;
        cv::GaussianBlur(workMat, workMat, cv::Size(k, k), 0);
    }
    if (m_chkBilateral->isChecked()) {
        cv::Mat tmp;
        cv::bilateralFilter(workMat, tmp, 7, 50, 50);
        workMat = tmp;
    }

    // 阶段 2: 二值化与边缘提取
    bool isBinaryOrEdge = false;
    if (m_chkCanny->isChecked()) {
        cv::Mat gray, edges;
        if (workMat.channels() > 1) cv::cvtColor(workMat, gray, cv::COLOR_BGR2GRAY);
        else gray = workMat;
        cv::Canny(gray, edges, m_cannyLowSlider->value(), m_cannyHighSlider->value());
        workMat = edges;
        isBinaryOrEdge = true;
    } else if (m_chkAdaptiveThresh->isChecked()) {
        cv::Mat gray, thresh;
        if (workMat.channels() > 1) cv::cvtColor(workMat, gray, cv::COLOR_BGR2GRAY);
        else gray = workMat;
        cv::adaptiveThreshold(gray, thresh, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);
        workMat = thresh;
        isBinaryOrEdge = true;
    }

    // 阶段 3: 形态学
    if (m_chkMorph->isChecked()) {
        int mk = m_morphKernelSlider->value();
        if (mk % 2 == 0) mk += 1;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(mk, mk));
        int op = m_morphTypeCombo->currentData().toInt();
        int cvOp = (op == 0 ? cv::MORPH_DILATE : (op == 1 ? cv::MORPH_ERODE : (op == 2 ? cv::MORPH_OPEN : cv::MORPH_CLOSE)));
        cv::morphologyEx(workMat, workMat, cvOp, kernel);
    }

    // 阶段 4: 质检分析与标注画板准备
    cv::Mat outBgr;
    if (workMat.channels() == 1) {
        cv::cvtColor(workMat, outBgr, cv::COLOR_GRAY2BGR);
    } else {
        outBgr = workMat.clone();
    }

    int defectCount = 0;
    double maxArea = 0.0;

    // 轮廓外接矩形与面积判等
    if (m_chkContours->isChecked()) {
        cv::Mat binForContours;
        if (workMat.channels() > 1) {
            cv::cvtColor(workMat, binForContours, cv::COLOR_BGR2GRAY);
            cv::threshold(binForContours, binForContours, 80, 255, cv::THRESH_BINARY);
        } else {
            binForContours = workMat.clone();
        }

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binForContours, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int minArea = m_minAreaSlider->value();
        for (const auto &cnt : contours) {
            double a = cv::contourArea(cnt);
            if (a >= minArea) {
                defectCount++;
                if (a > maxArea) maxArea = a;
                cv::Rect r = cv::boundingRect(cnt);
                // 红色高亮报警矩形框
                cv::rectangle(outBgr, r, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
                cv::putText(outBgr, fmt::format("#{}:{:.0f}", defectCount, a), 
                            cv::Point(r.x, qMax(16, r.y - 4)), 
                            cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
            }
        }
    }

    // ORB 特征关键点实时提取
    if (m_chkOrbKeypoints->isChecked()) {
        static auto orb = cv::ORB::create(150);
        std::vector<cv::KeyPoint> kpts;
        cv::Mat desc;
        cv::Mat grayForOrb;
        if (workMat.channels() > 1) cv::cvtColor(workMat, grayForOrb, cv::COLOR_BGR2GRAY);
        else grayForOrb = workMat;
        orb->detectAndCompute(grayForOrb, cv::noArray(), kpts, desc);
        cv::drawKeypoints(outBgr, kpts, outBgr, cv::Scalar(0, 255, 0), cv::DrawMatchesFlags::DRAW_OVER_OUTIMG);
    }

    // Harris 拐点标注
    if (m_chkHarrisCorners->isChecked()) {
        cv::Mat grayForHarris, hresp, dnorm;
        if (workMat.channels() > 1) cv::cvtColor(workMat, grayForHarris, cv::COLOR_BGR2GRAY);
        else grayForHarris = workMat;
        cv::cornerHarris(grayForHarris, hresp, 2, 3, 0.04);
        cv::normalize(hresp, dnorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);
        for (int j = 0; j < dnorm.rows; j += 4) {
            const float *row = dnorm.ptr<float>(j);
            for (int i = 0; i < dnorm.cols; i += 4) {
                if (row[i] > 145) {
                    cv::circle(outBgr, cv::Point(i, j), 4, cv::Scalar(0, 140, 255), -1, cv::LINE_AA);
                }
            }
        }
    }

    auto endT = std::chrono::high_resolution_clock::now();
    double latencyMs = std::chrono::duration<double, std::milli>(endT - startT).count();

    // FPS 统计更新
    m_frameCount++;
    m_fpsCounter++;
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastFpsCheck).count();
    if (elapsed >= 0.5) {
        m_currentFps = m_fpsCounter / elapsed;
        m_fpsCounter = 0;
        m_lastFpsCheck = now;
    }

    // 渲染双通道画廊
    QImage processedQImg = ScreenCapture::matToQImage(outBgr);
    QPixmap inPix = QPixmap::fromImage(rawImg).scaled(m_liveInputLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap outPix = QPixmap::fromImage(processedQImg).scaled(m_liveOutputLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_liveInputLabel->setPixmap(inPix);
    m_liveOutputLabel->setPixmap(outPix);

    // 判定逻辑
    bool passed = (defectCount == 0);
    if (passed) {
        m_livePassBadgeLabel->setText("✅ 质检判定: PASS (无超标缺陷)");
        m_livePassBadgeLabel->setStyleSheet("background-color: #16a34a; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; font-size: 12px;");
    } else {
        m_livePassBadgeLabel->setText(QString("❌ 质检判定: NG (检出 %1 处目标)").arg(defectCount));
        m_livePassBadgeLabel->setStyleSheet("background-color: #dc2626; color: white; padding: 4px 12px; border-radius: 4px; font-weight: bold; font-size: 12px;");
    }

    m_statusMetricsLabel->setText(QString("🟢 实时帧率: %1 FPS | 算法耗时: %2 ms\n"
                                          "📐 分辨率: %3x%4 | 目标数: %5")
                                          .arg(m_currentFps, 0, 'f', 1)
                                          .arg(latencyMs, 0, 'f', 1)
                                          .arg(inputBgr.cols)
                                          .arg(inputBgr.rows)
                                          .arg(defectCount));

    // 每 4 帧记录一行质检数据，保持表格灵敏且不过载
    if (m_frameCount % 4 == 0) {
        addInspectionRecord(m_frameCount, defectCount, maxArea, passed, latencyMs);
    }
}

void LiveScreenPipelinePage::addInspectionRecord(int frameNum, int objCount, double maxArea, bool passed, double latencyMs) {
    int row = m_logTable->rowCount();
    m_logTable->insertRow(row);

    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");

    auto *item0 = new QTableWidgetItem(QString("#%1").arg(frameNum));
    auto *item1 = new QTableWidgetItem(timeStr);
    auto *item2 = new QTableWidgetItem(QString("%1 个").arg(objCount));
    auto *item3 = new QTableWidgetItem(QString("%1 px²").arg(static_cast<int>(maxArea)));
    auto *item4 = new QTableWidgetItem(passed ? "PASS" : "FAIL (NG)");
    auto *item5 = new QTableWidgetItem(QString("%1 ms").arg(latencyMs, 0, 'f', 1));

    item0->setTextAlignment(Qt::AlignCenter);
    item1->setTextAlignment(Qt::AlignCenter);
    item2->setTextAlignment(Qt::AlignCenter);
    item3->setTextAlignment(Qt::AlignCenter);
    item4->setTextAlignment(Qt::AlignCenter);
    item5->setTextAlignment(Qt::AlignCenter);

    if (passed) {
        item4->setForeground(QColor("#16a34a"));
    } else {
        item4->setForeground(QColor("#ef4444"));
        item4->setFont(QFont("", -1, QFont::Bold));
    }

    m_logTable->setItem(row, 0, item0);
    m_logTable->setItem(row, 1, item1);
    m_logTable->setItem(row, 2, item2);
    m_logTable->setItem(row, 3, item3);
    m_logTable->setItem(row, 4, item4);
    m_logTable->setItem(row, 5, item5);

    // 维持最多 40 条记录
    if (m_logTable->rowCount() > 40) {
        m_logTable->removeRow(0);
    }
    m_logTable->scrollToBottom();
}

void LiveScreenPipelinePage::clearHistoryTable() {
    m_logTable->setRowCount(0);
}

void LiveScreenPipelinePage::onActivated() {
    // 切换进入页面时，如已开启则恢复定时器
    if (m_isStreaming && m_streamTimer && !m_streamTimer->isActive()) {
        int interval = m_fpsCombo->currentData().toInt();
        m_streamTimer->start(interval);
    }
}

void LiveScreenPipelinePage::onDeactivated() {
    // 切出页面时自动暂停采集，节省 CPU 算力
    if (m_streamTimer && m_streamTimer->isActive()) {
        m_streamTimer->stop();
    }
}
