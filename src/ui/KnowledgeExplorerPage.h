#pragma once

#include "core/IToolPage.h"
#include "core/KnowledgeModel.h"
#include <QTreeWidget>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QMap>
#include <QTimer>
#include <opencv2/core.hpp>

class CppSyntaxHighlighter;

class KnowledgeExplorerPage : public IToolPage {
    Q_OBJECT
public:
    explicit KnowledgeExplorerPage(QWidget *parent = nullptr);
    ~KnowledgeExplorerPage() override = default;

    QString id() const override { return "knowledge_explorer"; }
    QString title() const override { return "Qt & OpenCV 交互知识实验室"; }
    QString description() const override { 
        return "数据驱动的全景方法学实验室：树形大纲导航、API 函数原型精讲、参数联动动态 C++ 代码生成与实时双屏算法比对。"; 
    }
    QString icon() const override { return "🎓"; }

    void onActivated() override;

private slots:
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void filterTreeCategory(int filterMode);
    void captureScreenSource();
    void openImageSource();
    void resetSyntheticSource();
    void copyCodeToClipboard();
    void exportCodeToFile();
    void copyApiSignature();
    void resetCurrentParams();
    void onParamChanged();

private:
    void setupUI();
    void populateKnowledgeTree();
    void loadTopic(const QString &topicId);
    void buildDynamicParamWidgets(const KnowledgeTopic &topic);
    void runCurrentAlgorithm();
    void generateSyntheticImage();

    // 体系分类筛选与多级索引大纲树
    QTreeWidget *m_treeWidget = nullptr;
    QLabel *m_topicCountBadge = nullptr;
    int m_currentTreeFilter = 0; // 0: All, 1: OpenCV, 2: Qt, 3: Interactive, 4: Guide

    // 计算防抖定时器 (保证滑块 60 FPS 满帧顺滑)
    QTimer *m_debounceTimer = nullptr;

    // 当前选中的知识点数据
    QString m_currentTopicId;
    QMap<QString, QVariant> m_currentParams;

    // 文档区
    QLabel *m_topicTitleLabel = nullptr;
    QLabel *m_topicTagLabel = nullptr;
    QLabel *m_apiSignatureLabel = nullptr;
    QPushButton *m_copyApiBtn = nullptr;
    QLabel *m_docSummaryLabel = nullptr;
    QLabel *m_docParamsLabel = nullptr;

    // 代码生成与语法高亮区
    QTextEdit *m_codeEdit = nullptr;
    QPushButton *m_copyCodeBtn = nullptr;
    QPushButton *m_exportCodeBtn = nullptr;
    CppSyntaxHighlighter *m_codeHighlighter = nullptr;

    // 中下部视窗堆叠：分为「视觉实时算法对比页」与「深度架构与使用时机指南页」
    QStackedWidget *m_contentStack = nullptr;

    // 页面 A：视觉交互视窗 (Visual Runner)
    QWidget *m_visualViewWidget = nullptr;
    QWidget *m_paramContainer = nullptr;
    QVBoxLayout *m_paramLayout = nullptr;
    cv::Mat m_sourceMat;
    cv::Mat m_resultMat;
    QLabel *m_srcPreviewLabel = nullptr;
    QLabel *m_dstPreviewLabel = nullptr;
    QLabel *m_perfBadgeLabel = nullptr;

    // 页面 B：非视觉机制与架构指南页 (Non-visual Guide)
    QWidget *m_guideViewWidget = nullptr;
    QLabel *m_timingLabel = nullptr;
    QLabel *m_pitfallsLabel = nullptr;
    QTextEdit *m_fullCodeEdit = nullptr;
    CppSyntaxHighlighter *m_fullCodeHighlighter = nullptr;
};
