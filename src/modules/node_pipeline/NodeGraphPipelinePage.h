#pragma once

#include "core/IToolPage.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSlider>
#include <QCheckBox>
#include <QTimer>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

class VisionNodeItem;
class ConnectionWireItem;

class NodeGraphPipelinePage : public IToolPage {
    Q_OBJECT
public:
    explicit NodeGraphPipelinePage(QWidget *parent = nullptr);
    ~NodeGraphPipelinePage() override;

    QString id() const override { return "node_pipeline"; }
    QString title() const override { return "节点图视觉流水线"; }
    QString icon() const override { return "🧩"; }
    QString description() const override {
        return "基于 QGraphicsView 的工业视觉算子连线编排工作站，支持鼠标拖拽节点、动态贝塞尔曲线连接、实时屏幕流式拓扑演算。";
    }

    void onActivated() override;
    void onDeactivated() override;

private slots:
    void onSceneSelectionChanged();
    void executePipeline();
    void toggleContinuousStream();
    void loadPresetWorkflow();
    void clearCanvas();

private:
    void setupUI();
    void initTopToolbar(QVBoxLayout *rootLayout);
    void setupRightInspector(QHBoxLayout *mainLayout);
    
    // 节点创建助手
    VisionNodeItem* addNode(int type, const QString &title, const QPointF &pos);
    void connectNodes(VisionNodeItem *source, VisionNodeItem *target);

    // 选定节点参数面板更新
    void updateInspectorForNode(VisionNodeItem *node);

    QGraphicsView *m_view = nullptr;
    QGraphicsScene *m_scene = nullptr;

    // 连续流定时器
    QTimer *m_streamTimer = nullptr;
    bool m_isStreaming = false;
    QPushButton *m_streamBtn = nullptr;
    QLabel *m_pipelineStatsLabel = nullptr;

    // 右侧属性与大图监视面板
    QFrame *m_inspectorPanel = nullptr;
    QLabel *m_inspectorTitle = nullptr;
    QLabel *m_inspectorTypeBadge = nullptr;
    QWidget *m_paramsContainer = nullptr;
    QVBoxLayout *m_paramsLayout = nullptr;
    QLabel *m_previewImageLabel = nullptr;
    QLabel *m_previewMetricsLabel = nullptr;

    VisionNodeItem *m_selectedNode = nullptr;
    QList<VisionNodeItem*> m_nodes;
    QList<ConnectionWireItem*> m_wires;
};
