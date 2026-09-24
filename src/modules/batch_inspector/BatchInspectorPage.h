#pragma once

#include "core/IToolPage.h"
#include <QTableWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <opencv2/core.hpp>

struct BatchInspectionItem {
    int id = 0;
    QString sampleName;
    cv::Mat originalMat;
    cv::Mat annotatedMat;
    bool isPassed = true;
    double measuredHoleDiameter = 0.0;
    double defectArea = 0.0;
    QString failReason;
    double latencyMs = 0.0;
    quintptr threadId = 0;
};

class BatchInspectorPage : public IToolPage {
    Q_OBJECT
public:
    explicit BatchInspectorPage(QWidget *parent = nullptr);
    ~BatchInspectorPage() override = default;

    QString id() const override { return "batch_inspector"; }
    QString title() const override { return "工业质检批处理中心"; }
    QString icon() const override { return "🏭"; }
    QString description() const override {
        return "基于 QtConcurrent 多核并行架构的高吞吐质检测评工坊，提供虚拟工件批次生成、多线程并行瑕疵检测、良品率测评与数据报表导出。";
    }

    void onActivated() override;

private slots:
    void generateSyntheticBatch();
    void loadLocalFolder();
    void runBatchInspection();
    void onTableRowSelected();
    void filterTable(int filterType); // 0: All, 1: PASS, 2: NG
    void exportCsvReport();

private:
    void setupUI();
    void initLeftControlPanel(QHBoxLayout *mainLayout);
    void initCenterTableView(QHBoxLayout *mainLayout);
    void initRightViewer(QHBoxLayout *mainLayout);

    // 图像检测单件执行体
    static BatchInspectionItem inspectSingleItem(BatchInspectionItem item, double targetDia, double tolDia, double maxDefectA);

    // 界面控件
    QPushButton *m_runBatchBtn = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QSlider *m_threadSlider = nullptr;
    QLabel *m_threadValLabel = nullptr;
    QSlider *m_toleranceSlider = nullptr;
    QLabel *m_toleranceValLabel = nullptr;

    // 宏观统计指示牌
    QLabel *m_totalCountLabel = nullptr;
    QLabel *m_passCountLabel = nullptr;
    QLabel *m_ngCountLabel = nullptr;
    QLabel *m_yieldRateLabel = nullptr;
    QLabel *m_avgLatencyLabel = nullptr;

    // 流水明细表
    QTableWidget *m_table = nullptr;
    int m_currentFilter = 0; // 0: All, 1: Pass, 2: NG

    // 右侧视觉质检对比画廊
    QLabel *m_rightDetailTitle = nullptr;
    QLabel *m_rightPassBadge = nullptr;
    QLabel *m_rightOrigLabel = nullptr;
    QLabel *m_rightAnnotatedLabel = nullptr;
    QLabel *m_rightMetricsLabel = nullptr;

    QList<BatchInspectionItem> m_items;
    bool m_isRunning = false;
};
