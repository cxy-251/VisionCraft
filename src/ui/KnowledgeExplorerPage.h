#pragma once

#include "core/IToolPage.h"
#include "core/KnowledgeModel.h"
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMap>
#include <opencv2/core.hpp>

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
    void onSearchTextChanged(const QString &text);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void captureScreenSource();
    void openImageSource();
    void resetSyntheticSource();
    void copyCodeToClipboard();
    void onParamChanged();

private:
    void setupUI();
    void populateKnowledgeTree();
    void loadTopic(const QString &topicId);
    void buildDynamicParamWidgets(const KnowledgeTopic &topic);
    void runCurrentAlgorithm();
    void generateSyntheticImage();

    // 搜索与树
    QLineEdit *m_searchEdit = nullptr;
    QTreeWidget *m_treeWidget = nullptr;

    // 当前选中的知识点数据
    QString m_currentTopicId;
    QMap<QString, QVariant> m_currentParams;

    // 文档区
    QLabel *m_topicTitleLabel = nullptr;
    QLabel *m_topicTagLabel = nullptr;
    QLabel *m_apiSignatureLabel = nullptr;
    QLabel *m_docSummaryLabel = nullptr;
    QLabel *m_docParamsLabel = nullptr;

    // 代码生成区
    QTextEdit *m_codeEdit = nullptr;
    QPushButton *m_copyCodeBtn = nullptr;

    // 动态参数容器
    QWidget *m_paramContainer = nullptr;
    QVBoxLayout *m_paramLayout = nullptr;

    // 图像数据源与视窗
    cv::Mat m_sourceMat;
    cv::Mat m_resultMat;
    QLabel *m_srcPreviewLabel = nullptr;
    QLabel *m_dstPreviewLabel = nullptr;
    QLabel *m_perfBadgeLabel = nullptr;
};
