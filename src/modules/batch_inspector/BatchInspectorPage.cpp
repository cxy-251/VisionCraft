#include "BatchInspectorPage.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QThreadPool>
#include <QThread>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <chrono>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

BatchInspectorPage::BatchInspectorPage(QWidget *parent) : IToolPage(parent) {
    setupUI();
    generateSyntheticBatch();
}

void BatchInspectorPage::setupUI() {
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 12, 15, 15);
    rootLayout->setSpacing(12);

    // 1. 左侧：控制面板与容差参数设置 (固定宽度 300)
    initLeftControlPanel(rootLayout);

    // 2. 中间：宏观指标指示牌 + 质检记录流水明细表 (弹性主视窗)
    initCenterTableView(rootLayout);

    // 3. 右侧：选定工件双路透视对比图谱 (固定宽度 340)
    initRightViewer(rootLayout);
}

void BatchInspectorPage::initLeftControlPanel(QHBoxLayout *mainLayout) {
    auto *panel = new QFrame(this);
    panel->setObjectName("PanelCard");
    panel->setFixedWidth(300);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto *title = new QLabel("⚙️ 质检批处理控制台", panel);
    title->setObjectName("CardTitle");
    layout->addWidget(title);

    // 数据源按钮组
    auto *genBtn = new QPushButton("🎲 1键生成虚拟批次样本 (24件)", panel);
    genBtn->setObjectName("SecondaryBtn");
    connect(genBtn, &QPushButton::clicked, this, &BatchInspectorPage::generateSyntheticBatch);
    layout->addWidget(genBtn);

    auto *folderBtn = new QPushButton("📂 载入本地工件图片目录...", panel);
    folderBtn->setObjectName("SecondaryBtn");
    connect(folderBtn, &QPushButton::clicked, this, &BatchInspectorPage::loadLocalFolder);
    layout->addWidget(folderBtn);

    // 多核线程池滑条
    int maxThreads = std::max(2, QThread::idealThreadCount());
    auto *threadRow = new QHBoxLayout();
    auto *threadTitle = new QLabel("并发工作线程池:", panel);
    threadTitle->setObjectName("CardSubTitle");
    m_threadValLabel = new QLabel(QString("%1 线程").arg(maxThreads), panel);
    m_threadValLabel->setStyleSheet("font-weight: bold; color: #38bdf8; font-size: 12px;");
    threadRow->addWidget(threadTitle);
    threadRow->addStretch();
    threadRow->addWidget(m_threadValLabel);
    layout->addLayout(threadRow);

    m_threadSlider = new QSlider(Qt::Horizontal, panel);
    m_threadSlider->setRange(1, maxThreads);
    m_threadSlider->setValue(maxThreads);
    connect(m_threadSlider, &QSlider::valueChanged, this, [this](int v) {
        m_threadValLabel->setText(QString("%1 线程").arg(v));
    });
    layout->addWidget(m_threadSlider);

    // 允许孔径尺寸公差滑块
    auto *tolRow = new QHBoxLayout();
    auto *tolTitle = new QLabel("主孔径允许公差 (±):", panel);
    tolTitle->setObjectName("CardSubTitle");
    m_toleranceValLabel = new QLabel("±8 px", panel);
    m_toleranceValLabel->setStyleSheet("font-weight: bold; color: #10b981; font-size: 12px;");
    tolRow->addWidget(tolTitle);
    tolRow->addStretch();
    tolRow->addWidget(m_toleranceValLabel);
    layout->addLayout(tolRow);

    m_toleranceSlider = new QSlider(Qt::Horizontal, panel);
    m_toleranceSlider->setRange(2, 25);
    m_toleranceSlider->setValue(8);
    connect(m_toleranceSlider, &QSlider::valueChanged, this, [this](int v) {
        m_toleranceValLabel->setText(QString("±%1 px").arg(v));
    });
    layout->addWidget(m_toleranceSlider);

    // 并行批处理启动按钮
    m_runBatchBtn = new QPushButton("🚀 启动多核并行批处理", panel);
    m_runBatchBtn->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    connect(m_runBatchBtn, &QPushButton::clicked, this, &BatchInspectorPage::runBatchInspection);
    layout->addWidget(m_runBatchBtn);

    // 进度条
    m_progressBar = new QProgressBar(panel);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #0f172a; border-radius: 4px; }"
        "QProgressBar::chunk { background-color: #10b981; border-radius: 4px; }"
    );
    layout->addWidget(m_progressBar);

    // 报表导出按钮
    auto *exportBtn = new QPushButton("📊 导出 CSV 质检明细报表", panel);
    exportBtn->setStyleSheet(
        "QPushButton { background-color: #0d9488; color: white; border: none; border-radius: 6px; padding: 9px; font-weight: 600; font-size: 12px; }"
        "QPushButton:hover { background-color: #0f766e; }"
    );
    connect(exportBtn, &QPushButton::clicked, this, &BatchInspectorPage::exportCsvReport);
    layout->addWidget(exportBtn);

    layout->addStretch();
    mainLayout->addWidget(panel);
}

void BatchInspectorPage::initCenterTableView(QHBoxLayout *mainLayout) {
    auto *centerWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // 1. 顶部宏观指标指示卡牌 (KPI Cards Row)
    auto *kpiCard = new QFrame(centerWidget);
    kpiCard->setObjectName("PanelCard");
    auto *kpiLayout = new QHBoxLayout(kpiCard);
    kpiLayout->setContentsMargins(14, 12, 14, 12);
    kpiLayout->setSpacing(10);

    auto makeMetricBox = [](const QString &label, QLabel *&valLabel, const QString &defaultVal, const QString &color) -> QWidget* {
        auto *w = new QWidget();
        auto *l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);
        auto *sub = new QLabel(label, w);
        sub->setStyleSheet("font-size: 11px; color: #94a3b8;");
        valLabel = new QLabel(defaultVal, w);
        valLabel->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;").arg(color));
        l->addWidget(sub);
        l->addWidget(valLabel);
        return w;
    };

    kpiLayout->addWidget(makeMetricBox("总抽检批次", m_totalCountLabel, "24 件", "#f8fafc"));
    kpiLayout->addWidget(makeMetricBox("合格良品 (PASS)", m_passCountLabel, "0 件", "#10b981"));
    kpiLayout->addWidget(makeMetricBox("不良品 (NG)", m_ngCountLabel, "0 件", "#ef4444"));
    kpiLayout->addWidget(makeMetricBox("综合良品率", m_yieldRateLabel, "--%", "#38bdf8"));
    kpiLayout->addWidget(makeMetricBox("单件平均算法耗时", m_avgLatencyLabel, "-- ms", "#f59e0b"));

    layout->addWidget(kpiCard);

    // 2. 质检记录流水明细表卡片
    auto *tableCard = new QFrame(centerWidget);
    tableCard->setObjectName("PanelCard");
    auto *tblLayout = new QVBoxLayout(tableCard);
    tblLayout->setContentsMargins(14, 12, 14, 12);
    tblLayout->setSpacing(8);

    // 过滤按钮栏
    auto *filterRow = new QHBoxLayout();
    auto *tblTitle = new QLabel("📋 质检明细流水表", tableCard);
    tblTitle->setObjectName("CardTitle");
    filterRow->addWidget(tblTitle);
    filterRow->addStretch();

    auto *btnAll = new QPushButton("全部工件", tableCard);
    btnAll->setObjectName("SecondaryBtn");
    connect(btnAll, &QPushButton::clicked, this, [this]() { filterTable(0); });
    filterRow->addWidget(btnAll);

    auto *btnPass = new QPushButton("仅合格 (PASS)", tableCard);
    btnPass->setObjectName("SecondaryBtn");
    connect(btnPass, &QPushButton::clicked, this, [this]() { filterTable(1); });
    filterRow->addWidget(btnPass);

    auto *btnNG = new QPushButton("仅超差 (NG)", tableCard);
    btnNG->setObjectName("SecondaryBtn");
    connect(btnNG, &QPushButton::clicked, this, [this]() { filterTable(2); });
    filterRow->addWidget(btnNG);

    tblLayout->addLayout(filterRow);

    m_table = new QTableWidget(tableCard);
    m_table->setColumnCount(7);
    m_table->setHorizontalHeaderLabels({"工件编号", "样本来源", "判定结论", "测得主孔径", "瑕疵面积", "检测耗时", "调度线程"});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setStyleSheet(
        "QTableWidget { background-color: #0b1329; color: #f8fafc; border: 1px solid #334155; border-radius: 6px; gridline-color: #1e293b; font-size: 12px; }"
        "QHeaderView::section { background-color: #1e293b; color: #94a3b8; font-weight: 600; border: none; padding: 6px; border-bottom: 1px solid #334155; }"
        "QTableWidget::item:selected { background-color: #2563eb; color: #ffffff; }"
    );
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &BatchInspectorPage::onTableRowSelected);
    tblLayout->addWidget(m_table, 1);

    layout->addWidget(tableCard, 1);
    mainLayout->addWidget(centerWidget, 1);
}

void BatchInspectorPage::initRightViewer(QHBoxLayout *mainLayout) {
    auto *panel = new QFrame(this);
    panel->setObjectName("PanelCard");
    panel->setFixedWidth(340);

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    auto *headerRow = new QHBoxLayout();
    m_rightDetailTitle = new QLabel("工件测量透视图", panel);
    m_rightDetailTitle->setObjectName("CardTitle");
    m_rightPassBadge = new QLabel("就绪", panel);
    m_rightPassBadge->setStyleSheet("background-color: #0284c7; color: white; border-radius: 4px; padding: 3px 8px; font-size: 11px; font-weight: bold;");
    headerRow->addWidget(m_rightDetailTitle);
    headerRow->addWidget(m_rightPassBadge);
    headerRow->addStretch();
    layout->addLayout(headerRow);

    // 1. 原始图像预览
    auto *origTitle = new QLabel("1️⃣ 原始工件采样 (Original)", panel);
    origTitle->setObjectName("CardSubTitle");
    layout->addWidget(origTitle);

    m_rightOrigLabel = new QLabel("等待载入...", panel);
    m_rightOrigLabel->setAlignment(Qt::AlignCenter);
    m_rightOrigLabel->setFixedHeight(140);
    m_rightOrigLabel->setStyleSheet("background-color: #0f172a; border: 1px solid #334155; border-radius: 6px; color: #64748b; font-size: 12px;");
    layout->addWidget(m_rightOrigLabel);

    // 2. 算法特征识别标注图
    auto *annTitle = new QLabel("2️⃣ 算法尺寸标定与瑕疵透视图", panel);
    annTitle->setObjectName("CardSubTitle");
    layout->addWidget(annTitle);

    m_rightAnnotatedLabel = new QLabel("等待检测...", panel);
    m_rightAnnotatedLabel->setAlignment(Qt::AlignCenter);
    m_rightAnnotatedLabel->setFixedHeight(140);
    m_rightAnnotatedLabel->setStyleSheet("background-color: #0f172a; border: 1px solid #334155; border-radius: 6px; color: #64748b; font-size: 12px;");
    layout->addWidget(m_rightAnnotatedLabel);

    // 3. 详细读数卡片
    m_rightMetricsLabel = new QLabel("请在左侧列表中点击任一工件行查看质检透视参数。", panel);
    m_rightMetricsLabel->setObjectName("StatusBox");
    m_rightMetricsLabel->setWordWrap(true);
    layout->addWidget(m_rightMetricsLabel);

    layout->addStretch();
    mainLayout->addWidget(panel);
}

void BatchInspectorPage::generateSyntheticBatch() {
    m_items.clear();

    for (int i = 1; i <= 24; ++i) {
        BatchInspectionItem item;
        item.id = i;
        item.sampleName = QString("WORKPIECE_SN_%1").arg(1000 + i);

        // 生成底图
        cv::Mat mat(360, 540, CV_8UC3, cv::Scalar(205, 210, 215));
        cv::rectangle(mat, cv::Rect(40, 30, 460, 300), cv::Scalar(155, 160, 165), -1);
        cv::rectangle(mat, cv::Rect(40, 30, 460, 300), cv::Scalar(70, 75, 80), 3);

        // 四周标准安装孔
        cv::circle(mat, cv::Point(90, 80), 14, cv::Scalar(45, 45, 45), -1);
        cv::circle(mat, cv::Point(450, 80), 14, cv::Scalar(45, 45, 45), -1);
        cv::circle(mat, cv::Point(90, 280), 14, cv::Scalar(45, 45, 45), -1);
        cv::circle(mat, cv::Point(450, 280), 14, cv::Scalar(45, 45, 45), -1);

        // 主装配孔（标准直径约为 110px，即半径 55px）
        int holeRadius = 55;
        // 人为在特定工件中植入真实物理缺陷
        if (i == 4 || i == 11) {
            holeRadius = 66; // 孔径超大严重 NG
        } else if (i == 8 || i == 19) {
            holeRadius = 45; // 孔径偏小严重 NG
        }

        cv::circle(mat, cv::Point(270, 180), holeRadius, cv::Scalar(35, 40, 45), -1);

        // 植入划痕或脏污瑕疵
        if (i == 6 || i == 15 || i == 22) {
            cv::line(mat, cv::Point(290, 100), cv::Point(420, 220), cv::Scalar(25, 25, 210), 3); // 红色严重划痕
        } else if (i == 17) {
            cv::circle(mat, cv::Point(160, 180), 12, cv::Scalar(20, 20, 190), -1); // 异物杂质
        }

        item.originalMat = mat;
        item.annotatedMat = mat.clone();
        m_items.append(item);
    }

    runBatchInspection();
}

void BatchInspectorPage::loadLocalFolder() {
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择本地工件图片目录");
    if (dirPath.isEmpty()) return;

    QDir dir(dirPath);
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.bmp" << "*.jpeg" << "*.tif";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files);

    if (fileList.isEmpty()) {
        QMessageBox::information(this, "提示", "所选目录中未发现可识别的图像文件！");
        return;
    }

    m_items.clear();
    int count = std::min(static_cast<int>(fileList.size()), 60); // 批处理上限 60 件
    for (int i = 0; i < count; ++i) {
        BatchInspectionItem item;
        item.id = i + 1;
        item.sampleName = fileList[i].fileName();
        item.originalMat = cv::imread(fileList[i].absoluteFilePath().toLocal8Bit().constData(), cv::IMREAD_COLOR);
        if (!item.originalMat.empty()) {
            item.annotatedMat = item.originalMat.clone();
            m_items.append(item);
        }
    }

    runBatchInspection();
}

BatchInspectionItem BatchInspectorPage::inspectSingleItem(BatchInspectionItem item, double targetDia, double tolDia, double maxDefectA) {
    auto t1 = std::chrono::high_resolution_clock::now();
    item.threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());

    cv::Mat gray;
    cv::cvtColor(item.originalMat, gray, cv::COLOR_BGR2GRAY);

    // 二值化求得内部暗孔
    cv::Mat bin;
    cv::threshold(gray, bin, 70, 255, cv::THRESH_BINARY_INV);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(bin, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxHoleRadius = 0.0;
    cv::Point2f holeCenter(0, 0);
    double detectedDefectArea = 0.0;

    for (const auto &c : contours) {
        double a = cv::contourArea(c);
        if (a > 1500) { // 中心孔
            cv::Point2f center;
            float r;
            cv::minEnclosingCircle(c, center, r);
            if (r > maxHoleRadius) {
                maxHoleRadius = r;
                holeCenter = center;
            }
        } else if (a > 40 && a < 1200) { // 瑕疵/划痕/毛刺
            detectedDefectArea += a;
            cv::Rect defectRect = cv::boundingRect(c);
            cv::rectangle(item.annotatedMat, defectRect, cv::Scalar(0, 0, 255), 2);
            cv::putText(item.annotatedMat, "DEFECT", cv::Point(defectRect.x, defectRect.y - 4),
                        cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 255), 1);
        }
    }

    double measuredDia = maxHoleRadius * 2.0;
    item.measuredHoleDiameter = measuredDia;
    item.defectArea = detectedDefectArea;

    // 判定逻辑
    bool diaOk = std::abs(measuredDia - targetDia) <= tolDia;
    bool defectOk = (detectedDefectArea <= maxDefectA);

    if (diaOk && defectOk) {
        item.isPassed = true;
        item.failReason = "检验合格 (ALL OK)";
        // 标注绿色中心孔合格圈
        cv::circle(item.annotatedMat, cv::Point(static_cast<int>(holeCenter.x), static_cast<int>(holeCenter.y)),
                   static_cast<int>(maxHoleRadius), cv::Scalar(0, 255, 0), 2);
        cv::putText(item.annotatedMat, QString("Dia:%1px [PASS]").arg(measuredDia, 0, 'f', 1).toStdString(),
                    cv::Point(static_cast<int>(holeCenter.x - 70), static_cast<int>(holeCenter.y + 5)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 2);
    } else {
        item.isPassed = false;
        if (!diaOk && !defectOk) {
            item.failReason = QString("孔径超差 (%1px) & 表面缺陷 (%2px²)").arg(measuredDia, 0, 'f', 1).arg(static_cast<int>(detectedDefectArea));
        } else if (!diaOk) {
            item.failReason = QString("孔径尺寸超差 (%1px 偏离标准 %2px)").arg(measuredDia, 0, 'f', 1).arg(targetDia, 0, 'f', 1);
        } else {
            item.failReason = QString("检出表面划痕/脏污缺陷 (%1 px²)").arg(static_cast<int>(detectedDefectArea));
        }
        // 标注红色中心孔异常圈
        cv::circle(item.annotatedMat, cv::Point(static_cast<int>(holeCenter.x), static_cast<int>(holeCenter.y)),
                   static_cast<int>(maxHoleRadius), cv::Scalar(0, 0, 255), 3);
        cv::putText(item.annotatedMat, QString("Dia:%1px [NG]").arg(measuredDia, 0, 'f', 1).toStdString(),
                    cv::Point(static_cast<int>(holeCenter.x - 65), static_cast<int>(holeCenter.y + 5)),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 2);
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    item.latencyMs = std::chrono::duration<double, std::milli>(t2 - t1).count();
    return item;
}

void BatchInspectorPage::runBatchInspection() {
    if (m_isRunning || m_items.isEmpty()) return;
    m_isRunning = true;
    m_runBatchBtn->setEnabled(false);
    m_runBatchBtn->setText("⏳ 多核并发质检运算中...");
    m_progressBar->setValue(20);

    double targetDia = 110.0;
    double tolDia = m_toleranceSlider->value();
    double maxDefectA = 80.0;

    // 配置线程池
    int threads = m_threadSlider->value();
    QThreadPool::globalInstance()->setMaxThreadCount(threads);

    auto itemsCopy = m_items;
    
    // QtConcurrent 多核并行映射
    auto *watcher = new QFutureWatcher<BatchInspectionItem>(this);
    connect(watcher, &QFutureWatcher<BatchInspectionItem>::finished, this, [this, watcher]() {
        QFuture<BatchInspectionItem> future = watcher->future();
        m_items = future.results();
        watcher->deleteLater();

        m_progressBar->setValue(100);
        m_isRunning = false;
        m_runBatchBtn->setEnabled(true);
        m_runBatchBtn->setText("🚀 启动多核并行批处理");

        // 统计汇总
        int total = m_items.size();
        int passCount = 0;
        double sumLatency = 0.0;
        for (const auto &it : m_items) {
            if (it.isPassed) passCount++;
            sumLatency += it.latencyMs;
        }
        int ngCount = total - passCount;
        double yield = total > 0 ? (passCount * 100.0 / total) : 0.0;
        double avgLat = total > 0 ? (sumLatency / total) : 0.0;

        m_totalCountLabel->setText(QString("%1 件").arg(total));
        m_passCountLabel->setText(QString("%1 件").arg(passCount));
        m_ngCountLabel->setText(QString("%1 件").arg(ngCount));
        m_yieldRateLabel->setText(QString("%1%").arg(yield, 0, 'f', 1));
        m_yieldRateLabel->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1;")
                                       .arg(yield >= 85.0 ? "#10b981" : (yield >= 70.0 ? "#f59e0b" : "#ef4444")));
        m_avgLatencyLabel->setText(QString("%1 ms").arg(avgLat, 0, 'f', 2));

        filterTable(m_currentFilter);

        // 默认选中第一行展示
        if (m_table->rowCount() > 0) {
            m_table->selectRow(0);
        }
    });

    QFuture<BatchInspectionItem> future = QtConcurrent::mapped(
        itemsCopy, [targetDia, tolDia, maxDefectA](const BatchInspectionItem &item) {
            return inspectSingleItem(item, targetDia, tolDia, maxDefectA);
        });

    watcher->setFuture(future);
}

void BatchInspectorPage::filterTable(int filterType) {
    m_currentFilter = filterType;
    m_table->setRowCount(0);

    for (int i = 0; i < m_items.size(); ++i) {
        const auto &it = m_items[i];
        if (m_currentFilter == 1 && !it.isPassed) continue; // 仅 PASS
        if (m_currentFilter == 2 && it.isPassed) continue;  // 仅 NG

        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto *item0 = new QTableWidgetItem(QString("#%1").arg(it.id));
        auto *item1 = new QTableWidgetItem(it.sampleName);
        auto *item2 = new QTableWidgetItem(it.isPassed ? "PASS (良品)" : "FAIL (NG)");
        auto *item3 = new QTableWidgetItem(QString("%1 px").arg(it.measuredHoleDiameter, 0, 'f', 1));
        auto *item4 = new QTableWidgetItem(it.defectArea > 0 ? QString("%1 px²").arg(static_cast<int>(it.defectArea)) : "无");
        auto *item5 = new QTableWidgetItem(QString("%1 ms").arg(it.latencyMs, 0, 'f', 2));
        auto *item6 = new QTableWidgetItem(QString("Thread 0x%1").arg(it.threadId, 0, 16));

        item0->setTextAlignment(Qt::AlignCenter);
        item1->setTextAlignment(Qt::AlignCenter);
        item2->setTextAlignment(Qt::AlignCenter);
        item3->setTextAlignment(Qt::AlignCenter);
        item4->setTextAlignment(Qt::AlignCenter);
        item5->setTextAlignment(Qt::AlignCenter);
        item6->setTextAlignment(Qt::AlignCenter);

        if (it.isPassed) {
            item2->setForeground(QColor("#10b981"));
        } else {
            item2->setForeground(QColor("#ef4444"));
            item2->setFont(QFont("", -1, QFont::Bold));
        }

        m_table->setItem(row, 0, item0);
        m_table->setItem(row, 1, item1);
        m_table->setItem(row, 2, item2);
        m_table->setItem(row, 3, item3);
        m_table->setItem(row, 4, item4);
        m_table->setItem(row, 5, item5);
        m_table->setItem(row, 6, item6);

        // 存储该行对应的 item index
        item0->setData(Qt::UserRole, i);
    }
}

void BatchInspectorPage::onTableRowSelected() {
    int curRow = m_table->currentRow();
    if (curRow < 0) return;

    auto *item0 = m_table->item(curRow, 0);
    if (!item0) return;
    int idx = item0->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_items.size()) return;

    const auto &item = m_items[idx];

    m_rightDetailTitle->setText(QString("工件 #%1 透视").arg(item.id));
    if (item.isPassed) {
        m_rightPassBadge->setText("✅ 合格 (PASS)");
        m_rightPassBadge->setStyleSheet("background-color: #10b981; color: white; border-radius: 4px; padding: 3px 8px; font-size: 11px; font-weight: bold;");
    } else {
        m_rightPassBadge->setText("❌ 超差 (NG)");
        m_rightPassBadge->setStyleSheet("background-color: #ef4444; color: white; border-radius: 4px; padding: 3px 8px; font-size: 11px; font-weight: bold;");
    }

    auto matToPixmap = [](const cv::Mat &mat, int w, int h) -> QPixmap {
        if (mat.empty()) return QPixmap();
        cv::Mat resized;
        cv::resize(mat, resized, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);
        cv::Mat rgb;
        cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
        QImage qimg(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888);
        return QPixmap::fromImage(qimg);
    };

    m_rightOrigLabel->setPixmap(matToPixmap(item.originalMat, 310, 140));
    m_rightAnnotatedLabel->setPixmap(matToPixmap(item.annotatedMat, 310, 140));

    m_rightMetricsLabel->setText(
        QString("📌 工件代码: %1\n"
                "📐 测得主孔径: %2 px (标准: 110±%3 px)\n"
                "🔍 表面缺陷面积: %4 px²\n"
                "⏱️ 算法耗时: %5 ms (Thread 0x%6)\n"
                "📝 判定结论: %7")
        .arg(item.sampleName)
        .arg(item.measuredHoleDiameter, 0, 'f', 1)
        .arg(m_toleranceSlider->value())
        .arg(static_cast<int>(item.defectArea))
        .arg(item.latencyMs, 0, 'f', 2)
        .arg(item.threadId, 0, 16)
        .arg(item.failReason)
    );
}

void BatchInspectorPage::exportCsvReport() {
    if (m_items.isEmpty()) {
        QMessageBox::information(this, "提示", "当前无质检记录可导出！");
        return;
    }

    QString defaultName = QString("VisionCraft_QC_Report_%1.csv")
                          .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "导出 CSV 质检明细与统计算法报表", defaultName, "CSV 文件 (*.csv)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法创建目标报表文件！");
        return;
    }

    QTextStream out(&file);
    // 写入 UTF-8 BOM，确保 Excel 默认直接打开绝不乱码
    out << "\xEF\xBB\xBF";

    // 写入报表头
    out << "VisionCraft 工业机器视觉多核并行批处理质检报表\n";
    out << QString("生成时间,%1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    out << QString("抽检总数,%1\n").arg(m_totalCountLabel->text());
    out << QString("良品数,%1\n").arg(m_passCountLabel->text());
    out << QString("不良品数,%1\n").arg(m_ngCountLabel->text());
    out << QString("良品率,%1\n").arg(m_yieldRateLabel->text());
    out << QString("平均单件耗时,%1\n\n").arg(m_avgLatencyLabel->text());

    // 写入明细表头
    out << "工件编号,工件标识SN,判定结果,测得主孔径(px),表面缺陷面积(px²),算法耗时(ms),调度线程号,判定原因说明\n";
    for (const auto &it : m_items) {
        out << QString("%1,%2,%3,%4,%5,%6,0x%7,\"%8\"\n")
               .arg(it.id)
               .arg(it.sampleName)
               .arg(it.isPassed ? "PASS" : "NG")
               .arg(it.measuredHoleDiameter, 0, 'f', 2)
               .arg(static_cast<int>(it.defectArea))
               .arg(it.latencyMs, 0, 'f', 2)
               .arg(it.threadId, 0, 16)
               .arg(it.failReason);
    }

    file.close();
    QMessageBox::information(this, "导出成功", QString("质检报表已成功生成并写入:\n%1").arg(filePath));
}

void BatchInspectorPage::onActivated() {
    if (m_items.isEmpty()) {
        generateSyntheticBatch();
    }
}
