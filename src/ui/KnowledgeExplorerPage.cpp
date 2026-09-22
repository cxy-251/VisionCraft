#include "KnowledgeExplorerPage.h"
#include "core/KnowledgeRegistry.h"
#include "core/ScreenCapture.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFrame>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QClipboard>
#include <QTimer>
#include <QGuiApplication>
#include <chrono>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

KnowledgeExplorerPage::KnowledgeExplorerPage(QWidget *parent) : IToolPage(parent) {
    setupUI();
    generateSyntheticImage();
    populateKnowledgeTree();
}

void KnowledgeExplorerPage::setupUI() {
    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(15, 15, 15, 15);
    rootLayout->setSpacing(0);

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);

    // ========================================================
    // 1. 左侧：系统化知识树与搜索栏 (固定宽度 ~300)
    // ========================================================
    auto *leftNavWidget = new QWidget(mainSplitter);
    leftNavWidget->setMinimumWidth(260);
    leftNavWidget->setMaximumWidth(340);
    leftNavWidget->setStyleSheet("background-color: #ffffff; border-right: 1px solid #e2e8f0;");

    auto *leftNavLayout = new QVBoxLayout(leftNavWidget);
    leftNavLayout->setContentsMargins(14, 14, 14, 14);
    leftNavLayout->setSpacing(10);

    auto *searchHeader = new QLabel("📚 知识体系导航", leftNavWidget);
    searchHeader->setStyleSheet("font-size: 15px; font-weight: 700; color: #0f172a;");
    leftNavLayout->addWidget(searchHeader);

    m_searchEdit = new QLineEdit(leftNavWidget);
    m_searchEdit->setPlaceholderText("🔍 搜索 API / 算法 (如 Canny, Blur)...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { padding: 8px 10px; border: 1px solid #cbd5e1; border-radius: 6px; font-size: 12px; background: #f8fafc; }"
        "QLineEdit:focus { border: 1px solid #2563eb; background: #ffffff; }"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, &KnowledgeExplorerPage::onSearchTextChanged);
    leftNavLayout->addWidget(m_searchEdit);

    m_treeWidget = new QTreeWidget(leftNavWidget);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(16);
    m_treeWidget->setStyleSheet(
        "QTreeWidget { border: none; font-size: 13px; color: #334155; }"
        "QTreeWidget::item { padding: 6px 4px; border-radius: 4px; }"
        "QTreeWidget::item:hover { background-color: #f1f5f9; }"
        "QTreeWidget::item:selected { background-color: #e0e7ff; color: #1e40af; font-weight: 600; }"
    );
    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &KnowledgeExplorerPage::onTreeItemClicked);
    leftNavLayout->addWidget(m_treeWidget, 1);

    mainSplitter->addWidget(leftNavWidget);

    // ========================================================
    // 2. 右侧：全景学习操作台
    // ========================================================
    auto *rightWorkbench = new QWidget(mainSplitter);
    rightWorkbench->setStyleSheet("background-color: #f8fafc;");
    auto *wbLayout = new QVBoxLayout(rightWorkbench);
    wbLayout->setContentsMargins(20, 15, 20, 15);
    wbLayout->setSpacing(14);

    // --- 顶部：API 知识点文档卡片 ---
    auto *docCard = new QFrame(rightWorkbench);
    docCard->setStyleSheet("QFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *docLayout = new QVBoxLayout(docCard);
    docLayout->setContentsMargins(18, 16, 18, 16);
    docLayout->setSpacing(8);

    auto *titleRow = new QHBoxLayout();
    m_topicTitleLabel = new QLabel("请选择知识点", docCard);
    m_topicTitleLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #0f172a; border: none;");
    m_topicTagLabel = new QLabel("", docCard);
    m_topicTagLabel->setStyleSheet("font-size: 12px; padding: 2px 8px; border-radius: 4px; background: #e0f2fe; color: #0369a1; font-weight: 600; border: none;");
    titleRow->addWidget(m_topicTitleLabel);
    titleRow->addWidget(m_topicTagLabel);
    titleRow->addStretch();
    docLayout->addLayout(titleRow);

    // API 签名显示框
    m_apiSignatureLabel = new QLabel(docCard);
    m_apiSignatureLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_apiSignatureLabel->setStyleSheet(
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; font-weight: 600; background: #0f172a; color: #38bdf8; padding: 10px 14px; border-radius: 6px; border: none;"
    );
    docLayout->addWidget(m_apiSignatureLabel);

    // 原理与参数说明
    m_docSummaryLabel = new QLabel(docCard);
    m_docSummaryLabel->setWordWrap(true);
    m_docSummaryLabel->setStyleSheet("font-size: 13px; color: #334155; line-height: 1.5; border: none;");
    docLayout->addWidget(m_docSummaryLabel);

    m_docParamsLabel = new QLabel(docCard);
    m_docParamsLabel->setWordWrap(true);
    m_docParamsLabel->setStyleSheet("font-size: 12px; color: #64748b; line-height: 1.5; border: none;");
    docLayout->addWidget(m_docParamsLabel);

    wbLayout->addWidget(docCard);

    // --- 中间：实时动态 C++ 代码生成区 ---
    auto *codeCard = new QFrame(rightWorkbench);
    codeCard->setStyleSheet("QFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *codeLayout = new QVBoxLayout(codeCard);
    codeLayout->setContentsMargins(16, 12, 16, 12);
    codeLayout->setSpacing(8);

    auto *codeHeader = new QHBoxLayout();
    auto *codeTitle = new QLabel("💻 实时动态 C++ 调用代码 (参数已绑定，即拷即用)", codeCard);
    codeTitle->setStyleSheet("font-size: 13px; font-weight: 600; color: #0f172a; border: none;");
    m_copyCodeBtn = new QPushButton("📋 复制代码", codeCard);
    m_copyCodeBtn->setCursor(Qt::PointingHandCursor);
    m_copyCodeBtn->setStyleSheet(
        "QPushButton { background: #2563eb; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; font-weight: 500; }"
        "QPushButton:hover { background: #1d4ed8; }"
    );
    connect(m_copyCodeBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::copyCodeToClipboard);
    codeHeader->addWidget(codeTitle);
    codeHeader->addStretch();
    codeHeader->addWidget(m_copyCodeBtn);
    codeLayout->addLayout(codeHeader);

    m_codeEdit = new QTextEdit(codeCard);
    m_codeEdit->setReadOnly(true);
    m_codeEdit->setFixedHeight(95);
    m_codeEdit->setStyleSheet(
        "QTextEdit { font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; background: #1e293b; color: #a5f3fc; border: 1px solid #334155; border-radius: 6px; padding: 6px; }"
    );
    codeLayout->addWidget(m_codeEdit);
    wbLayout->addWidget(codeCard);

    // --- 底部：参数交互面板 (左) + 实时对比视窗 (右) ---
    auto *bottomSplit = new QHBoxLayout();
    bottomSplit->setSpacing(14);

    // 左侧：动态参数容器
    auto *paramCard = new QFrame(rightWorkbench);
    paramCard->setFixedWidth(310);
    paramCard->setStyleSheet("QFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *paramRootLayout = new QVBoxLayout(paramCard);
    paramRootLayout->setContentsMargins(14, 14, 14, 14);

    auto *pTitle = new QLabel("🎛️ 算法实参实时微调", paramCard);
    pTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #0f172a; border: none;");
    paramRootLayout->addWidget(pTitle);

    auto *paramScroll = new QScrollArea(paramCard);
    paramScroll->setWidgetResizable(true);
    paramScroll->setFrameShape(QFrame::NoFrame);
    paramScroll->setStyleSheet("background: transparent; border: none;");

    m_paramContainer = new QWidget(paramScroll);
    m_paramLayout = new QVBoxLayout(m_paramContainer);
    m_paramLayout->setContentsMargins(0, 5, 0, 5);
    m_paramLayout->setSpacing(12);
    paramScroll->setWidget(m_paramContainer);
    paramRootLayout->addWidget(paramScroll, 1);

    // 输入源切换操作按钮行
    auto *srcBtnRow = new QVBoxLayout();
    srcBtnRow->setSpacing(6);
    auto *grabBtn = new QPushButton("📸 捕获当前屏幕作为输入源", paramCard);
    grabBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px; font-size: 12px; }"
                           "QPushButton:hover { background: #e2e8f0; }");
    connect(grabBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::captureScreenSource);

    auto *openImgBtn = new QPushButton("📂 打开本地图片", paramCard);
    openImgBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px; font-size: 12px; }"
                             "QPushButton:hover { background: #e2e8f0; }");
    connect(openImgBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::openImageSource);

    auto *resetImgBtn = new QPushButton("🖼️ 恢复标准测试色卡", paramCard);
    resetImgBtn->setStyleSheet("QPushButton { background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px; font-size: 12px; }"
                              "QPushButton:hover { background: #e2e8f0; }");
    connect(resetImgBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::resetSyntheticSource);

    srcBtnRow->addWidget(grabBtn);
    srcBtnRow->addWidget(openImgBtn);
    srcBtnRow->addWidget(resetImgBtn);
    paramRootLayout->addLayout(srcBtnRow);

    bottomSplit->addWidget(paramCard);

    // 右侧：双屏对比画廊
    auto *galleryCard = new QFrame(rightWorkbench);
    galleryCard->setStyleSheet("QFrame { background: #ffffff; border: 1px solid #e2e8f0; border-radius: 10px; }");
    auto *galleryLayout = new QVBoxLayout(galleryCard);
    galleryLayout->setContentsMargins(14, 14, 14, 14);
    galleryLayout->setSpacing(10);

    auto *galHeader = new QHBoxLayout();
    auto *galTitle = new QLabel("🖥️ 算法实时效果比对视窗", galleryCard);
    galTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #0f172a; border: none;");
    m_perfBadgeLabel = new QLabel("⚡ 准备就绪", galleryCard);
    m_perfBadgeLabel->setStyleSheet("font-size: 12px; color: #16a34a; font-weight: 600; border: none;");
    galHeader->addWidget(galTitle);
    galHeader->addStretch();
    galHeader->addWidget(m_perfBadgeLabel);
    galleryLayout->addLayout(galHeader);

    auto *splitView = new QHBoxLayout();
    splitView->setSpacing(10);

    // 原图视窗
    auto *srcBox = new QFrame(galleryCard);
    srcBox->setStyleSheet("background: #0f172a; border-radius: 6px; border: none;");
    auto *srcBoxLay = new QVBoxLayout(srcBox);
    auto *srcTag = new QLabel("原始输入图", srcBox);
    srcTag->setStyleSheet("color: #94a3b8; font-size: 11px;");
    m_srcPreviewLabel = new QLabel(srcBox);
    m_srcPreviewLabel->setAlignment(Qt::AlignCenter);
    srcBoxLay->addWidget(srcTag);
    srcBoxLay->addWidget(m_srcPreviewLabel, 1);

    // 效果图视窗
    auto *dstBox = new QFrame(galleryCard);
    dstBox->setStyleSheet("background: #0f172a; border-radius: 6px; border: none;");
    auto *dstBoxLay = new QVBoxLayout(dstBox);
    auto *dstTag = new QLabel("算法处理输出", dstBox);
    dstTag->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: bold;");
    m_dstPreviewLabel = new QLabel(dstBox);
    m_dstPreviewLabel->setAlignment(Qt::AlignCenter);
    dstBoxLay->addWidget(dstTag);
    dstBoxLay->addWidget(m_dstPreviewLabel, 1);

    splitView->addWidget(srcBox, 1);
    splitView->addWidget(dstBox, 1);
    galleryLayout->addLayout(splitView, 1);

    bottomSplit->addWidget(galleryCard, 1);
    wbLayout->addLayout(bottomSplit, 1);

    mainSplitter->addWidget(rightWorkbench);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    rootLayout->addWidget(mainSplitter);
}

void KnowledgeExplorerPage::generateSyntheticImage() {
    // 自动生成一张 640x480 的标准测试图（带渐变、圆、文字、矩形，极其适合测试各种算法）
    m_sourceMat = cv::Mat(480, 640, CV_8UC3);
    for (int y = 0; y < 480; ++y) {
        for (int x = 0; x < 640; ++x) {
            m_sourceMat.at<cv::Vec3b>(y, x) = cv::Vec3b(
                static_cast<uchar>(x * 255 / 640),
                static_cast<uchar>(y * 255 / 480),
                128
            );
        }
    }
    // 绘制几何图形
    cv::circle(m_sourceMat, cv::Point(200, 240), 90, cv::Scalar(0, 255, 0), -1); // 绿色实心圆
    cv::circle(m_sourceMat, cv::Point(200, 240), 100, cv::Scalar(255, 255, 255), 3);
    cv::rectangle(m_sourceMat, cv::Rect(380, 140, 180, 160), cv::Scalar(0, 165, 255), -1); // 橙色矩形
    cv::putText(m_sourceMat, "VisionCraft Standard Test Card", cv::Point(60, 60), 
                cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    cv::putText(m_sourceMat, "Qt 6 + OpenCV 4 Interactive Lab", cv::Point(80, 420), 
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2, cv::LINE_AA);
}

void KnowledgeExplorerPage::populateKnowledgeTree() {
    m_treeWidget->clear();
    const auto &grouped = KnowledgeRegistry::instance().topicsByCategory();

    QTreeWidgetItem *firstTopicItem = nullptr;

    for (auto it = grouped.begin(); it != grouped.end(); ++it) {
        auto *categoryItem = new QTreeWidgetItem(m_treeWidget);
        categoryItem->setText(0, it.key());
        categoryItem->setFlags(categoryItem->flags() & ~Qt::ItemIsSelectable);
        categoryItem->setFont(0, QFont("", -1, QFont::Bold));
        categoryItem->setForeground(0, QColor("#1e293b"));

        for (const auto &topic : it.value()) {
            auto *topicItem = new QTreeWidgetItem(categoryItem);
            topicItem->setText(0, QString("%1  [%2]").arg(topic.name, topic.tag));
            topicItem->setData(0, Qt::UserRole, topic.id);

            if (!firstTopicItem) {
                firstTopicItem = topicItem;
            }
        }
        categoryItem->setExpanded(true);
    }

    if (firstTopicItem) {
        m_treeWidget->setCurrentItem(firstTopicItem);
        loadTopic(firstTopicItem->data(0, Qt::UserRole).toString());
    }
}

void KnowledgeExplorerPage::onSearchTextChanged(const QString &text) {
    QString q = text.trimmed();
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        auto *catItem = m_treeWidget->topLevelItem(i);
        bool anyChildVisible = false;
        for (int j = 0; j < catItem->childCount(); ++j) {
            auto *child = catItem->child(j);
            bool match = q.isEmpty() || child->text(0).contains(q, Qt::CaseInsensitive);
            child->setHidden(!match);
            if (match) anyChildVisible = true;
        }
        catItem->setHidden(!anyChildVisible);
        if (anyChildVisible && !q.isEmpty()) {
            catItem->setExpanded(true);
        }
    }
}

void KnowledgeExplorerPage::onTreeItemClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item) return;
    QString topicId = item->data(0, Qt::UserRole).toString();
    if (!topicId.isEmpty()) {
        loadTopic(topicId);
    }
}

void KnowledgeExplorerPage::loadTopic(const QString &topicId) {
    const auto *topic = KnowledgeRegistry::instance().findTopic(topicId);
    if (!topic) return;

    m_currentTopicId = topicId;
    m_currentParams.clear();

    // 1. 设置文档展示
    m_topicTitleLabel->setText(topic->name);
    m_topicTagLabel->setText(topic->tag);
    m_apiSignatureLabel->setText(topic->apiSignature);
    m_docSummaryLabel->setText(topic->docSummary);
    m_docParamsLabel->setText(topic->docParams);

    // 2. 动态生成参数控件
    buildDynamicParamWidgets(*topic);

    // 3. 执行一次算法并更新代码
    runCurrentAlgorithm();
}

void KnowledgeExplorerPage::buildDynamicParamWidgets(const KnowledgeTopic &topic) {
    // 清理旧控件
    QLayoutItem *child;
    while ((child = m_paramLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    // 遍历参数定义动态生成
    for (const auto &p : topic.params) {
        m_currentParams[p.key] = p.defaultVal;

        auto *itemBox = new QWidget(m_paramContainer);
        auto *itemLay = new QVBoxLayout(itemBox);
        itemLay->setContentsMargins(0, 0, 0, 0);
        itemLay->setSpacing(4);

        if (p.type == ParamType::SliderInt || p.type == ParamType::SliderDouble) {
            auto *hRow = new QHBoxLayout();
            auto *nameLbl = new QLabel(p.label, itemBox);
            nameLbl->setStyleSheet("font-size: 12px; color: #334155; font-weight: 500; border: none;");
            auto *valLbl = new QLabel(QString::number(p.defaultVal), itemBox);
            valLbl->setStyleSheet("font-size: 12px; font-weight: bold; color: #2563eb; border: none;");
            hRow->addWidget(nameLbl);
            hRow->addStretch();
            hRow->addWidget(valLbl);
            itemLay->addLayout(hRow);

            auto *slider = new QSlider(Qt::Horizontal, itemBox);
            QString key = p.key;

            if (p.type == ParamType::SliderInt) {
                slider->setRange(static_cast<int>(p.minVal), static_cast<int>(p.maxVal));
                slider->setSingleStep(static_cast<int>(p.step));
                slider->setValue(static_cast<int>(p.defaultVal));
                connect(slider, &QSlider::valueChanged, this, [this, key, valLbl](int v) {
                    valLbl->setText(QString::number(v));
                    m_currentParams[key] = v;
                    onParamChanged();
                });
            } else {
                // 浮点数滑块 (放大 10 倍)
                slider->setRange(static_cast<int>(p.minVal * 10), static_cast<int>(p.maxVal * 10));
                slider->setValue(static_cast<int>(p.defaultVal * 10));
                connect(slider, &QSlider::valueChanged, this, [this, key, valLbl](int v) {
                    double actual = v / 10.0;
                    valLbl->setText(QString::number(actual, 'f', 1));
                    m_currentParams[key] = actual;
                    onParamChanged();
                });
            }
            itemLay->addWidget(slider);
        }
        else if (p.type == ParamType::ComboBox) {
            auto *nameLbl = new QLabel(p.label, itemBox);
            nameLbl->setStyleSheet("font-size: 12px; color: #334155; font-weight: 500; border: none;");
            itemLay->addWidget(nameLbl);

            auto *combo = new QComboBox(itemBox);
            for (int i = 0; i < p.options.size(); ++i) {
                int val = (i < p.optionValues.size()) ? p.optionValues[i] : i;
                combo->addItem(p.options[i], val);
            }
            QString key = p.key;
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, key, combo]() {
                m_currentParams[key] = combo->currentData();
                onParamChanged();
            });
            itemLay->addWidget(combo);
        }
        else if (p.type == ParamType::CheckBox) {
            auto *cb = new QCheckBox(p.label, itemBox);
            cb->setChecked(p.defaultVal > 0);
            QString key = p.key;
            connect(cb, &QCheckBox::toggled, this, [this, key](bool checked) {
                m_currentParams[key] = checked ? 1 : 0;
                onParamChanged();
            });
            itemLay->addWidget(cb);
        }

        m_paramLayout->addWidget(itemBox);
    }
    m_paramLayout->addStretch();
}

void KnowledgeExplorerPage::onParamChanged() {
    runCurrentAlgorithm();
}

void KnowledgeExplorerPage::runCurrentAlgorithm() {
    const auto *topic = KnowledgeRegistry::instance().findTopic(m_currentTopicId);
    if (!topic) return;

    // 1. 生成并同步 C++ 代码
    if (topic->codeGenerator) {
        m_codeEdit->setPlainText(topic->codeGenerator(m_currentParams));
    }

    // 2. 运行 OpenCV 算法
    if (topic->cvRunner && !m_sourceMat.empty()) {
        auto start = std::chrono::high_resolution_clock::now();
        QString note;
        topic->cvRunner(m_sourceMat, m_resultMat, m_currentParams, note);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        m_perfBadgeLabel->setText(QString("⚡ 算法耗时: %1 ms | %2").arg(QString::number(ms, 'f', 2), note));

        // 更新视窗画面
        QImage srcImg = ScreenCapture::matToQImage(m_sourceMat);
        QImage dstImg = ScreenCapture::matToQImage(m_resultMat);

        QSize viewSize = m_srcPreviewLabel->parentWidget()->size() - QSize(10, 30);
        m_srcPreviewLabel->setPixmap(QPixmap::fromImage(srcImg).scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_dstPreviewLabel->setPixmap(QPixmap::fromImage(dstImg).scaled(viewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void KnowledgeExplorerPage::captureScreenSource() {
    QImage img = ScreenCapture::grabScreen(0);
    if (!img.isNull()) {
        m_sourceMat = ScreenCapture::qImageToMat(img);
        runCurrentAlgorithm();
    }
}

void KnowledgeExplorerPage::openImageSource() {
    QString path = QFileDialog::getOpenFileName(this, "选择输入测试图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (path.isEmpty()) return;

    cv::Mat img = cv::imread(path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
    if (!img.empty()) {
        m_sourceMat = img;
        runCurrentAlgorithm();
    }
}

void KnowledgeExplorerPage::resetSyntheticSource() {
    generateSyntheticImage();
    runCurrentAlgorithm();
}

void KnowledgeExplorerPage::copyCodeToClipboard() {
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(m_codeEdit->toPlainText());
    m_copyCodeBtn->setText("✅ 已复制！");
    m_copyCodeBtn->setStyleSheet("QPushButton { background: #16a34a; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; font-weight: 500; }");
    QTimer::singleShot(1500, this, [this]() {
        m_copyCodeBtn->setText("📋 复制代码");
        m_copyCodeBtn->setStyleSheet("QPushButton { background: #2563eb; color: white; border: none; border-radius: 4px; padding: 4px 12px; font-size: 12px; font-weight: 500; }");
    });
}

void KnowledgeExplorerPage::onActivated() {
    runCurrentAlgorithm();
}
