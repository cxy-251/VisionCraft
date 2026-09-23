#include "KnowledgeExplorerPage.h"
#include "ThemeManager.h"
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
    rootLayout->setContentsMargins(10, 10, 10, 10);
    rootLayout->setSpacing(0);

    auto *mainSplitter = new QSplitter(Qt::Horizontal, this);

    // ========================================================
    // 1. 左侧：系统化知识树与搜索栏 (宽度 ~280)
    // ========================================================
    auto *leftNavWidget = new QWidget(mainSplitter);
    leftNavWidget->setMinimumWidth(260);
    leftNavWidget->setMaximumWidth(320);

    auto *leftNavLayout = new QVBoxLayout(leftNavWidget);
    leftNavLayout->setContentsMargins(10, 10, 10, 10);
    leftNavLayout->setSpacing(10);

    auto *searchHeader = new QLabel("📚 知识体系大纲", leftNavWidget);
    searchHeader->setStyleSheet("font-size: 15px; font-weight: 700;");
    leftNavLayout->addWidget(searchHeader);

    m_searchEdit = new QLineEdit(leftNavWidget);
    m_searchEdit->setPlaceholderText("🔍 搜索 API / 语法 (如 Canny, 信号槽)...");
    m_searchEdit->setStyleSheet(
        "QLineEdit { padding: 8px 10px; border-radius: 6px; font-size: 13px; }"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, &KnowledgeExplorerPage::onSearchTextChanged);
    leftNavLayout->addWidget(m_searchEdit);

    m_treeWidget = new QTreeWidget(leftNavWidget);
    m_treeWidget->setHeaderHidden(true);
    m_treeWidget->setIndentation(16);
    connect(m_treeWidget, &QTreeWidget::itemClicked, this, &KnowledgeExplorerPage::onTreeItemClicked);
    leftNavLayout->addWidget(m_treeWidget, 1);

    mainSplitter->addWidget(leftNavWidget);

    // ========================================================
    // 2. 右侧：全景学习操作台
    // ========================================================
    auto *rightScroll = new QScrollArea(mainSplitter);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);

    auto *rightWorkbench = new QWidget(rightScroll);
    auto *wbLayout = new QVBoxLayout(rightWorkbench);
    wbLayout->setContentsMargins(15, 10, 15, 15);
    wbLayout->setSpacing(12);

    // --- 顶部：API 知识点文档卡片 ---
    auto *docCard = new QFrame(rightWorkbench);
    docCard->setObjectName("PanelCard");
    auto *docLayout = new QVBoxLayout(docCard);
    docLayout->setContentsMargins(16, 14, 16, 14);
    docLayout->setSpacing(8);

    auto *titleRow = new QHBoxLayout();
    m_topicTitleLabel = new QLabel("请在左侧选择知识点", docCard);
    m_topicTitleLabel->setStyleSheet("font-size: 18px; font-weight: 700;");
    m_topicTagLabel = new QLabel("", docCard);
    m_topicTagLabel->setStyleSheet("font-size: 12px; padding: 2px 8px; border-radius: 4px; background: #0284c7; color: #ffffff; font-weight: 600;");
    titleRow->addWidget(m_topicTitleLabel);
    titleRow->addWidget(m_topicTagLabel);
    titleRow->addStretch();
    docLayout->addLayout(titleRow);

    // API 签名显示框
    m_apiSignatureLabel = new QLabel(docCard);
    m_apiSignatureLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_apiSignatureLabel->setWordWrap(true);
    m_apiSignatureLabel->setStyleSheet(
        "font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; font-weight: 600; background: #0f172a; color: #38bdf8; padding: 10px 14px; border-radius: 6px; border: 1px solid #334155;"
    );
    docLayout->addWidget(m_apiSignatureLabel);

    // 原理与参数说明（彻底放开高度，杜绝文字截断）
    m_docSummaryLabel = new QLabel(docCard);
    m_docSummaryLabel->setWordWrap(true);
    m_docSummaryLabel->setStyleSheet("font-size: 13px; line-height: 1.5;");
    docLayout->addWidget(m_docSummaryLabel);

    m_docParamsLabel = new QLabel(docCard);
    m_docParamsLabel->setWordWrap(true);
    auto updateDocParamsColor = [this](bool isDark) {
        if (m_docParamsLabel) {
            m_docParamsLabel->setStyleSheet(QString("font-size: 12px; line-height: 1.5; color: %1;").arg(isDark ? "#94a3b8" : "#475569"));
        }
    };
    updateDocParamsColor(ThemeManager::instance().isDarkMode());
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, updateDocParamsColor);
    docLayout->addWidget(m_docParamsLabel);

    wbLayout->addWidget(docCard);

    // --- 中间：实时动态 C++ 代码生成区 (仅针对视觉交互模式显示) ---
    auto *codeCard = new QFrame(rightWorkbench);
    codeCard->setObjectName("PanelCard");
    auto *codeLayout = new QVBoxLayout(codeCard);
    codeLayout->setContentsMargins(14, 10, 14, 10);
    codeLayout->setSpacing(6);

    auto *codeHeader = new QHBoxLayout();
    auto *codeTitle = new QLabel("💻 实时 C++ 调用代码 (参数联动 / 即拷即用)", codeCard);
    codeTitle->setStyleSheet("font-size: 13px; font-weight: 600;");
    m_copyCodeBtn = new QPushButton("📋 复制代码", codeCard);
    m_copyCodeBtn->setCursor(Qt::PointingHandCursor);
    connect(m_copyCodeBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::copyCodeToClipboard);
    codeHeader->addWidget(codeTitle);
    codeHeader->addStretch();
    codeHeader->addWidget(m_copyCodeBtn);
    codeLayout->addLayout(codeHeader);

    m_codeEdit = new QTextEdit(codeCard);
    m_codeEdit->setReadOnly(true);
    m_codeEdit->setFixedHeight(85);
    m_codeEdit->setStyleSheet(
        "QTextEdit { font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; background: #0b1329; color: #a5f3fc; border: 1px solid #334155; border-radius: 6px; padding: 6px; }"
    );
    codeLayout->addWidget(m_codeEdit);
    wbLayout->addWidget(codeCard);

    // --- 底部：自适应双形态容器 (QStackedWidget) ---
    m_contentStack = new QStackedWidget(rightWorkbench);

    // ========================================================
    // 形态 A：视觉交互视窗 (Visual Mode)
    // ========================================================
    m_visualViewWidget = new QWidget(m_contentStack);
    auto *vLayout = new QHBoxLayout(m_visualViewWidget);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(12);

    // 参数控制区 (左)
    auto *paramCard = new QFrame(m_visualViewWidget);
    paramCard->setObjectName("PanelCard");
    paramCard->setFixedWidth(300);
    auto *paramRootLayout = new QVBoxLayout(paramCard);
    paramRootLayout->setContentsMargins(12, 12, 12, 12);

    auto *pTitle = new QLabel("🎛️ 算法实参实时微调", paramCard);
    pTitle->setStyleSheet("font-size: 14px; font-weight: 700;");
    paramRootLayout->addWidget(pTitle);

    auto *paramScroll = new QScrollArea(paramCard);
    paramScroll->setWidgetResizable(true);
    paramScroll->setFrameShape(QFrame::NoFrame);
    paramScroll->setStyleSheet("background: transparent; border: none;");

    m_paramContainer = new QWidget(paramScroll);
    m_paramLayout = new QVBoxLayout(m_paramContainer);
    m_paramLayout->setContentsMargins(0, 5, 0, 5);
    m_paramLayout->setSpacing(10);
    paramScroll->setWidget(m_paramContainer);
    paramRootLayout->addWidget(paramScroll, 1);

    auto *srcBtnRow = new QVBoxLayout();
    srcBtnRow->setSpacing(6);
    auto *grabBtn = new QPushButton("📸 捕获屏幕作为输入源", paramCard);
    grabBtn->setStyleSheet("QPushButton { font-size: 12px; padding: 6px; }");
    connect(grabBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::captureScreenSource);

    auto *openImgBtn = new QPushButton("📂 打开本地测试图", paramCard);
    openImgBtn->setStyleSheet("QPushButton { font-size: 12px; padding: 6px; }");
    connect(openImgBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::openImageSource);

    auto *resetImgBtn = new QPushButton("🖼️ 恢复标准测试色卡", paramCard);
    resetImgBtn->setStyleSheet("QPushButton { font-size: 12px; padding: 6px; }");
    connect(resetImgBtn, &QPushButton::clicked, this, &KnowledgeExplorerPage::resetSyntheticSource);

    srcBtnRow->addWidget(grabBtn);
    srcBtnRow->addWidget(openImgBtn);
    srcBtnRow->addWidget(resetImgBtn);
    paramRootLayout->addLayout(srcBtnRow);

    vLayout->addWidget(paramCard);

    // 双屏画廊视窗 (右)
    auto *galleryCard = new QFrame(m_visualViewWidget);
    galleryCard->setObjectName("PanelCard");
    auto *galleryLayout = new QVBoxLayout(galleryCard);
    galleryLayout->setContentsMargins(12, 12, 12, 12);
    galleryLayout->setSpacing(8);

    auto *galHeader = new QHBoxLayout();
    auto *galTitle = new QLabel("🖥️ 算法实时效果比对视窗", galleryCard);
    galTitle->setStyleSheet("font-size: 14px; font-weight: 700;");
    m_perfBadgeLabel = new QLabel("⚡ 准备就绪", galleryCard);
    m_perfBadgeLabel->setStyleSheet("font-size: 12px; color: #10b981; font-weight: 600;");
    galHeader->addWidget(galTitle);
    galHeader->addStretch();
    galHeader->addWidget(m_perfBadgeLabel);
    galleryLayout->addLayout(galHeader);

    auto *splitView = new QHBoxLayout();
    splitView->setSpacing(8);

    auto *srcBox = new QFrame(galleryCard);
    srcBox->setStyleSheet("background: #0f172a; border-radius: 6px; border: 1px solid #334155;");
    auto *srcBoxLay = new QVBoxLayout(srcBox);
    auto *srcTag = new QLabel("原始输入图", srcBox);
    srcTag->setStyleSheet("color: #94a3b8; font-size: 11px;");
    m_srcPreviewLabel = new QLabel(srcBox);
    m_srcPreviewLabel->setAlignment(Qt::AlignCenter);
    srcBoxLay->addWidget(srcTag);
    srcBoxLay->addWidget(m_srcPreviewLabel, 1);

    auto *dstBox = new QFrame(galleryCard);
    dstBox->setStyleSheet("background: #0f172a; border-radius: 6px; border: 1px solid #334155;");
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

    vLayout->addWidget(galleryCard, 1);
    m_contentStack->addWidget(m_visualViewWidget);

    // ========================================================
    // 形态 B：深度架构与使用时机指南页 (Guide Mode)
    // ========================================================
    m_guideViewWidget = new QWidget(m_contentStack);
    auto *gLayout = new QVBoxLayout(m_guideViewWidget);
    gLayout->setContentsMargins(0, 0, 0, 0);
    gLayout->setSpacing(12);

    auto *timingCard = new QFrame(m_guideViewWidget);
    timingCard->setObjectName("PanelCard");
    auto *timingLay = new QVBoxLayout(timingCard);
    timingLay->setContentsMargins(14, 12, 14, 12);
    auto *tTitle = new QLabel("🎯 什么时候用？最佳应用场景与时机", timingCard);
    tTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #38bdf8;");
    m_timingLabel = new QLabel(timingCard);
    m_timingLabel->setWordWrap(true);
    m_timingLabel->setStyleSheet("font-size: 13px; line-height: 1.6;");
    timingLay->addWidget(tTitle);
    timingLay->addWidget(m_timingLabel);
    gLayout->addWidget(timingCard);

    auto *pitfallCard = new QFrame(m_guideViewWidget);
    pitfallCard->setObjectName("PanelCard");
    auto *pitfallLay = new QVBoxLayout(pitfallCard);
    pitfallLay->setContentsMargins(14, 12, 14, 12);
    auto *pWarnTitle = new QLabel("⚠️ 核心避坑指南与性能法则", pitfallCard);
    pWarnTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #f59e0b;");
    m_pitfallsLabel = new QLabel(pitfallCard);
    m_pitfallsLabel->setWordWrap(true);
    m_pitfallsLabel->setStyleSheet("font-size: 13px; line-height: 1.6;");
    pitfallLay->addWidget(pWarnTitle);
    pitfallLay->addWidget(m_pitfallsLabel);
    gLayout->addWidget(pitfallCard);

    auto *codeFullHeader = new QHBoxLayout();
    auto *codeFullTitle = new QLabel("📖 生产级完整 C++ 工程代码范式", codeFullCard);
    codeFullTitle->setStyleSheet("font-size: 14px; font-weight: 700; color: #10b981;");
    auto *copyFullBtn = new QPushButton("📋 复制范式代码", codeFullCard);
    copyFullBtn->setCursor(Qt::PointingHandCursor);
    connect(copyFullBtn, &QPushButton::clicked, this, [this, copyFullBtn]() {
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(m_fullCodeEdit->toPlainText());
        copyFullBtn->setText("✅ 已复制！");
        QTimer::singleShot(1500, this, [copyFullBtn]() {
            copyFullBtn->setText("📋 复制范式代码");
        });
    });
    codeFullHeader->addWidget(codeFullTitle);
    codeFullHeader->addStretch();
    codeFullHeader->addWidget(copyFullBtn);
    codeFullLay->addLayout(codeFullHeader);

    m_fullCodeEdit = new QTextEdit(codeFullCard);
    m_fullCodeEdit->setReadOnly(true);
    m_fullCodeEdit->setMinimumHeight(240);
    m_fullCodeEdit->setStyleSheet(
        "QTextEdit { font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; background: #0b1329; color: #a5f3fc; border: 1px solid #334155; border-radius: 6px; padding: 6px; }"
    );
    codeFullLay->addWidget(m_fullCodeEdit);
    gLayout->addWidget(codeFullCard);

    m_contentStack->addWidget(m_guideViewWidget);

    wbLayout->addWidget(m_contentStack, 1);

    rightScroll->setWidget(rightWorkbench);
    mainSplitter->addWidget(rightScroll);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    rootLayout->addWidget(mainSplitter);
}

void KnowledgeExplorerPage::generateSyntheticImage() {
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
    cv::circle(m_sourceMat, cv::Point(200, 240), 90, cv::Scalar(0, 255, 0), -1);
    cv::circle(m_sourceMat, cv::Point(200, 240), 100, cv::Scalar(255, 255, 255), 3);
    cv::rectangle(m_sourceMat, cv::Rect(380, 140, 180, 160), cv::Scalar(0, 165, 255), -1);
    cv::putText(m_sourceMat, "VisionCraft Standard Card", cv::Point(60, 60), 
                cv::FONT_HERSHEY_SIMPLEX, 0.9, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    cv::putText(m_sourceMat, "Qt 6 & OpenCV 4 Lab", cv::Point(140, 420), 
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

    m_topicTitleLabel->setText(topic->name);
    m_topicTagLabel->setText(topic->tag);
    m_apiSignatureLabel->setText(topic->apiSignature);
    m_docSummaryLabel->setText(topic->docSummary);
    m_docParamsLabel->setText(topic->docParams);

    if (topic->isVisualInteractive) {
        m_contentStack->setCurrentWidget(m_visualViewWidget);
        m_codeEdit->parentWidget()->setVisible(true);
        buildDynamicParamWidgets(*topic);
        runCurrentAlgorithm();
    } else {
        m_contentStack->setCurrentWidget(m_guideViewWidget);
        m_codeEdit->parentWidget()->setVisible(false);
        m_timingLabel->setText(topic->usageTiming.isEmpty() ? "暂无特定场景描述" : topic->usageTiming);
        m_pitfallsLabel->setText(topic->bestPractices.isEmpty() ? "遵循现代 C++ RAII 规范" : topic->bestPractices);
        m_fullCodeEdit->setPlainText(topic->codeSnippet);
    }
}

void KnowledgeExplorerPage::buildDynamicParamWidgets(const KnowledgeTopic &topic) {
    QLayoutItem *child;
    while ((child = m_paramLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    for (const auto &p : topic.params) {
        m_currentParams[p.key] = p.defaultVal;

        auto *itemBox = new QWidget(m_paramContainer);
        auto *itemLay = new QVBoxLayout(itemBox);
        itemLay->setContentsMargins(0, 0, 0, 0);
        itemLay->setSpacing(4);

        if (p.type == ParamType::SliderInt || p.type == ParamType::SliderDouble) {
            auto *hRow = new QHBoxLayout();
            auto *nameLbl = new QLabel(p.label, itemBox);
            nameLbl->setStyleSheet("font-size: 12px; font-weight: 500;");
            auto *valLbl = new QLabel(QString::number(p.defaultVal), itemBox);
            valLbl->setStyleSheet("font-size: 12px; font-weight: bold; color: #38bdf8;");
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
            nameLbl->setStyleSheet("font-size: 12px; font-weight: 500;");
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
    if (!topic || !topic->isVisualInteractive) return;

    if (topic->codeGenerator) {
        m_codeEdit->setPlainText(topic->codeGenerator(m_currentParams));
    }

    if (topic->cvRunner && !m_sourceMat.empty()) {
        auto start = std::chrono::high_resolution_clock::now();
        QString note;
        topic->cvRunner(m_sourceMat, m_resultMat, m_currentParams, note);
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();

        m_perfBadgeLabel->setText(QString("⚡ 算法耗时: %1 ms | %2").arg(QString::number(ms, 'f', 2), note));

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
    QString path = QFileDialog::getOpenFileName(this, "选择测试图片", "", "Images (*.png *.jpg *.jpeg *.bmp)");
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
    QTimer::singleShot(1500, this, [this]() {
        m_copyCodeBtn->setText("📋 复制代码");
    });
}

void KnowledgeExplorerPage::onActivated() {
    runCurrentAlgorithm();
}
