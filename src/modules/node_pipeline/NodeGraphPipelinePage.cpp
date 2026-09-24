#include "NodeGraphPipelinePage.h"
#include "VisionNodeItem.h"
#include "ConnectionWireItem.h"
#include "ThemeManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>
#include <QQueue>
#include <QSet>
#include <chrono>
#include <opencv2/imgproc.hpp>

NodeGraphPipelinePage::NodeGraphPipelinePage(QWidget *parent) : IToolPage(parent) {
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-200, -200, 2400, 1600);

    m_streamTimer = new QTimer(this);
    connect(m_streamTimer, &QTimer::timeout, this, &NodeGraphPipelinePage::executePipeline);

    setupUI();
    loadPresetWorkflow();
}

NodeGraphPipelinePage::~NodeGraphPipelinePage() {
    if (m_streamTimer && m_streamTimer->isActive()) {
        m_streamTimer->stop();
    }
}

void NodeGraphPipelinePage::setupUI() {
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(15, 12, 15, 15);
    rootLayout->setSpacing(10);

    // 1. 顶部操作工具栏
    initTopToolbar(rootLayout);

    // 2. 主体工作区（左侧画板视窗 + 右侧属性监视面板）
    auto *mainLayout = new QHBoxLayout();
    mainLayout->setSpacing(12);

    // 视窗 View
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::RubberBandDrag);
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setStyleSheet("background-color: #0b1329; border: 1px solid #334155; border-radius: 8px;");

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &NodeGraphPipelinePage::onSceneSelectionChanged);
    mainLayout->addWidget(m_view, 3);

    // 右侧属性监视面板
    setupRightInspector(mainLayout);

    rootLayout->addLayout(mainLayout, 1);
}

void NodeGraphPipelinePage::initTopToolbar(QVBoxLayout *rootLayout) {
    auto *toolCard = new QFrame(this);
    toolCard->setObjectName("PanelCard");
    auto *cardLayout = new QVBoxLayout(toolCard);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(8);

    // 第一行：添加算子节点
    auto *row1 = new QHBoxLayout();
    auto *nodeLabel = new QLabel("➕ 快速插入算子节点:", toolCard);
    nodeLabel->setStyleSheet("font-weight: 700; font-size: 13px; color: #38bdf8;");
    row1->addWidget(nodeLabel);

    auto addBtn = [this, row1, toolCard](const QString &text, int type, const QString &name) {
        auto *btn = new QPushButton(text, toolCard);
        btn->setObjectName("SecondaryBtn");
        btn->setStyleSheet("font-size: 12px; padding: 4px 8px;");
        connect(btn, &QPushButton::clicked, this, [this, type, name]() {
            QPointF center = m_view->mapToScene(m_view->viewport()->rect().center());
            addNode(type, name, center + QPointF(rand() % 60 - 30, rand() % 60 - 30));
        });
        row1->addWidget(btn);
    };

    addBtn("🖥️ 屏幕输入", static_cast<int>(VisionNodeType::ScreenCapture), "虚拟相机");
    addBtn("🎲 合成工件", static_cast<int>(VisionNodeType::SyntheticImage), "虚拟工件源");
    addBtn("⚫ 灰度化", static_cast<int>(VisionNodeType::Grayscale), "灰度转换");
    addBtn("💧 高斯滤波", static_cast<int>(VisionNodeType::GaussianBlur), "高斯降噪");
    addBtn("⚡ Canny边缘", static_cast<int>(VisionNodeType::CannyEdge), "Canny边缘");
    addBtn("🌓 阈值二值化", static_cast<int>(VisionNodeType::Threshold), "Otsu二值化");
    addBtn("🧱 形态学", static_cast<int>(VisionNodeType::Morphology), "形态学滤波");
    addBtn("🎯 轮廓质检", static_cast<int>(VisionNodeType::ContourInspection), "轮廓缺陷检测");
    addBtn("📱 二维码定位", static_cast<int>(VisionNodeType::QRCodeDetector), "二维码识别");

    row1->addStretch();
    cardLayout->addLayout(row1);

    // 第二行：执行与流式编排控制
    auto *row2 = new QHBoxLayout();
    
    auto *runOnceBtn = new QPushButton("▶ 单次推流演算", toolCard);
    runOnceBtn->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 6px; padding: 7px 14px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #1d4ed8; }"
    );
    connect(runOnceBtn, &QPushButton::clicked, this, &NodeGraphPipelinePage::executePipeline);
    row2->addWidget(runOnceBtn);

    m_streamBtn = new QPushButton("⚡ 开启连续屏幕流 (30 FPS)", toolCard);
    m_streamBtn->setStyleSheet(
        "QPushButton { background-color: #16a34a; color: white; border: none; border-radius: 6px; padding: 7px 14px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #15803d; }"
    );
    connect(m_streamBtn, &QPushButton::clicked, this, &NodeGraphPipelinePage::toggleContinuousStream);
    row2->addWidget(m_streamBtn);

    auto *presetBtn = new QPushButton("📋 载入质检预设工作流", toolCard);
    presetBtn->setObjectName("SecondaryBtn");
    connect(presetBtn, &QPushButton::clicked, this, &NodeGraphPipelinePage::loadPresetWorkflow);
    row2->addWidget(presetBtn);

    auto *clearBtn = new QPushButton("🗑️ 清空画布", toolCard);
    clearBtn->setObjectName("SecondaryBtn");
    connect(clearBtn, &QPushButton::clicked, this, &NodeGraphPipelinePage::clearCanvas);
    row2->addWidget(clearBtn);

    row2->addSpacing(16);
    m_pipelineStatsLabel = new QLabel("就绪。请在画布中自由连线或点击运行。", toolCard);
    m_pipelineStatsLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
    row2->addWidget(m_pipelineStatsLabel);

    row2->addStretch();
    cardLayout->addLayout(row2);

    rootLayout->addWidget(toolCard);
}

void NodeGraphPipelinePage::setupRightInspector(QHBoxLayout *mainLayout) {
    m_inspectorPanel = new QFrame(this);
    m_inspectorPanel->setObjectName("PanelCard");
    m_inspectorPanel->setFixedWidth(360);

    auto *panelLayout = new QVBoxLayout(m_inspectorPanel);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(10);

    // 标题行
    auto *titleRow = new QHBoxLayout();
    m_inspectorTitle = new QLabel("未选定节点", m_inspectorPanel);
    m_inspectorTitle->setObjectName("CardTitle");
    m_inspectorTypeBadge = new QLabel("", m_inspectorPanel);
    m_inspectorTypeBadge->setStyleSheet("background-color: #0284c7; color: white; border-radius: 4px; padding: 2px 6px; font-size: 11px; font-weight: bold;");
    titleRow->addWidget(m_inspectorTitle);
    titleRow->addWidget(m_inspectorTypeBadge);
    titleRow->addStretch();
    panelLayout->addLayout(titleRow);

    // 参数编辑区
    m_paramsContainer = new QWidget(m_inspectorPanel);
    m_paramsLayout = new QVBoxLayout(m_paramsContainer);
    m_paramsLayout->setContentsMargins(0, 0, 0, 0);
    m_paramsLayout->setSpacing(8);
    panelLayout->addWidget(m_paramsContainer);

    // 高清结果监视大图
    auto *previewTitle = new QLabel("🖥️ 选定算子输出视窗", m_inspectorPanel);
    previewTitle->setObjectName("CardSubTitle");
    panelLayout->addWidget(previewTitle);

    m_previewImageLabel = new QLabel("暂无预览", m_inspectorPanel);
    m_previewImageLabel->setAlignment(Qt::AlignCenter);
    m_previewImageLabel->setMinimumHeight(200);
    m_previewImageLabel->setStyleSheet("background-color: #0f172a; border: 1px solid #334155; border-radius: 6px; color: #64748b; font-size: 12px;");
    panelLayout->addWidget(m_previewImageLabel, 1);

    m_previewMetricsLabel = new QLabel("等待演算...", m_inspectorPanel);
    m_previewMetricsLabel->setObjectName("StatusBox");
    panelLayout->addWidget(m_previewMetricsLabel);

    mainLayout->addWidget(m_inspectorPanel);
}

VisionNodeItem* NodeGraphPipelinePage::addNode(int type, const QString &title, const QPointF &pos) {
    auto *node = new VisionNodeItem(static_cast<VisionNodeType>(type), title, pos);
    m_scene->addItem(node);
    m_nodes.append(node);
    return node;
}

void NodeGraphPipelinePage::connectNodes(VisionNodeItem *source, VisionNodeItem *target) {
    if (!source || !target || source == target) return;
    auto *wire = new ConnectionWireItem(source, target);
    m_scene->addItem(wire);
    m_wires.append(wire);
}

void NodeGraphPipelinePage::loadPresetWorkflow() {
    clearCanvas();

    // 工业视觉标准多阶段缺陷检测流水线
    auto *nodeInput = addNode(static_cast<int>(VisionNodeType::ScreenCapture), "虚拟相机", QPointF(50, 160));
    auto *nodeGray = addNode(static_cast<int>(VisionNodeType::Grayscale), "灰度转换", QPointF(290, 160));
    auto *nodeBlur = addNode(static_cast<int>(VisionNodeType::GaussianBlur), "高斯滤波", QPointF(530, 160));
    auto *nodeCanny = addNode(static_cast<int>(VisionNodeType::CannyEdge), "Canny 边缘", QPointF(770, 160));
    auto *nodeInspect = addNode(static_cast<int>(VisionNodeType::ContourInspection), "轮廓缺陷质检", QPointF(1010, 160));

    connectNodes(nodeInput, nodeGray);
    connectNodes(nodeGray, nodeBlur);
    connectNodes(nodeBlur, nodeCanny);
    connectNodes(nodeCanny, nodeInspect);

    // 选定终点质检节点
    nodeInspect->setSelected(true);
    m_selectedNode = nodeInspect;
    updateInspectorForNode(nodeInspect);

    executePipeline();
}

void NodeGraphPipelinePage::clearCanvas() {
    if (m_isStreaming) toggleContinuousStream();

    for (auto *wire : m_wires) {
        m_scene->removeItem(wire);
        delete wire;
    }
    m_wires.clear();

    for (auto *node : m_nodes) {
        m_scene->removeItem(node);
        delete node;
    }
    m_nodes.clear();

    m_selectedNode = nullptr;
    m_inspectorTitle->setText("未选定节点");
    m_inspectorTypeBadge->setText("");
    m_previewImageLabel->setText("画布已清空");
    m_previewMetricsLabel->setText("就绪");
}

void NodeGraphPipelinePage::executePipeline() {
    auto t1 = std::chrono::high_resolution_clock::now();

    // 1. 查找全部根节点（无输入引脚，或无上游入线）
    QList<VisionNodeItem*> roots;
    for (auto *n : m_nodes) {
        if (!n->hasInputSocket() || n->incomingWires().isEmpty()) {
            roots.append(n);
        }
    }

    // 2. 拓扑流水线推演
    QQueue<VisionNodeItem*> queue;
    for (auto *r : roots) {
        r->process(cv::Mat());
        queue.enqueue(r);
    }

    QSet<VisionNodeItem*> processed;
    int execCount = roots.size();

    while (!queue.isEmpty()) {
        auto *curr = queue.dequeue();
        const cv::Mat &outMat = curr->outputMat();

        for (auto *wire : curr->outgoingWires()) {
            auto *next = wire->endNode();
            if (next) {
                next->process(outMat);
                execCount++;
                if (!processed.contains(next)) {
                    processed.insert(next);
                    queue.enqueue(next);
                }
            }
        }
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

    m_pipelineStatsLabel->setText(QString("🟢 流水线执行成功: 演算 %1 个算子 | 全流程耗时: %2 ms")
                                  .arg(execCount).arg(totalMs, 0, 'f', 1));

    // 同步刷新右侧监视器
    if (m_selectedNode) {
        updateInspectorForNode(m_selectedNode);
    }
}

void NodeGraphPipelinePage::toggleContinuousStream() {
    if (m_isStreaming) {
        m_streamTimer->stop();
        m_isStreaming = false;
        m_streamBtn->setText("⚡ 开启连续屏幕流 (30 FPS)");
        m_streamBtn->setStyleSheet(
            "QPushButton { background-color: #16a34a; color: white; border: none; border-radius: 6px; padding: 7px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background-color: #15803d; }"
        );
    } else {
        m_streamTimer->start(33); // 30 FPS
        m_isStreaming = true;
        m_streamBtn->setText("⏹️ 停止实时流");
        m_streamBtn->setStyleSheet(
            "QPushButton { background-color: #dc2626; color: white; border: none; border-radius: 6px; padding: 7px 14px; font-weight: bold; font-size: 12px; }"
            "QPushButton:hover { background-color: #b91c1c; }"
        );
    }
}

void NodeGraphPipelinePage::onSceneSelectionChanged() {
    auto selectedItems = m_scene->selectedItems();
    VisionNodeItem *pickedNode = nullptr;
    for (auto *item : selectedItems) {
        if (auto *n = dynamic_cast<VisionNodeItem*>(item)) {
            pickedNode = n;
            break;
        }
    }

    m_selectedNode = pickedNode;
    if (m_selectedNode) {
        updateInspectorForNode(m_selectedNode);
    }
}

void NodeGraphPipelinePage::updateInspectorForNode(VisionNodeItem *node) {
    if (!node) return;

    m_inspectorTitle->setText(node->title());
    m_inspectorTypeBadge->setText(QString("耗时: %1ms").arg(node->lastLatencyMs(), 0, 'f', 1));

    // 清空现有参数控件
    QLayoutItem *child;
    while ((child = m_paramsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    // 根据不同节点类型动态添加参数控件
    switch (node->nodeType()) {
        case VisionNodeType::ScreenCapture: {
            auto *modeLabel = new QLabel("采集区域模式:", m_paramsContainer);
            modeLabel->setObjectName("CardSubTitle");
            auto *combo = new QComboBox(m_paramsContainer);
            combo->addItem("中心工作区 (Center 640x400)", "center");
            combo->addItem("鼠标指针跟随 (Mouse ROI)", "mouse");
            combo->addItem("主屏幕全景 (Full Desktop)", "full");
            combo->setCurrentIndex(combo->findData(node->param("mode", "center")));
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, node, combo](int) {
                node->setParam("mode", combo->currentData().toString());
                executePipeline();
            });
            m_paramsLayout->addWidget(modeLabel);
            m_paramsLayout->addWidget(combo);
            break;
        }

        case VisionNodeType::GaussianBlur: {
            int curK = node->param("ksize", 7).toInt();
            auto *kRow = new QHBoxLayout();
            auto *kTitle = new QLabel(QString("滤波核尺寸 (ksize): %1").arg(curK), m_paramsContainer);
            kTitle->setObjectName("CardSubTitle");
            auto *slider = new QSlider(Qt::Horizontal, m_paramsContainer);
            slider->setRange(1, 15);
            slider->setValue((curK - 1) / 2);
            connect(slider, &QSlider::valueChanged, this, [this, node, kTitle](int val) {
                int k = val * 2 + 1;
                kTitle->setText(QString("滤波核尺寸 (ksize): %1").arg(k));
                node->setParam("ksize", k);
                executePipeline();
            });
            kRow->addWidget(kTitle);
            m_paramsLayout->addLayout(kRow);
            m_paramsLayout->addWidget(slider);
            break;
        }

        case VisionNodeType::CannyEdge: {
            int low = node->param("lowThresh", 50).toInt();
            int high = node->param("highThresh", 140).toInt();

            auto *lowLabel = new QLabel(QString("低阈值 (Threshold 1): %1").arg(low), m_paramsContainer);
            lowLabel->setObjectName("CardSubTitle");
            auto *slider1 = new QSlider(Qt::Horizontal, m_paramsContainer);
            slider1->setRange(10, 200);
            slider1->setValue(low);
            connect(slider1, &QSlider::valueChanged, this, [this, node, lowLabel](int v) {
                lowLabel->setText(QString("低阈值 (Threshold 1): %1").arg(v));
                node->setParam("lowThresh", v);
                executePipeline();
            });
            m_paramsLayout->addWidget(lowLabel);
            m_paramsLayout->addWidget(slider1);

            auto *highLabel = new QLabel(QString("高阈值 (Threshold 2): %1").arg(high), m_paramsContainer);
            highLabel->setObjectName("CardSubTitle");
            auto *slider2 = new QSlider(Qt::Horizontal, m_paramsContainer);
            slider2->setRange(50, 255);
            slider2->setValue(high);
            connect(slider2, &QSlider::valueChanged, this, [this, node, highLabel](int v) {
                highLabel->setText(QString("高阈值 (Threshold 2): %1").arg(v));
                node->setParam("highThresh", v);
                executePipeline();
            });
            m_paramsLayout->addWidget(highLabel);
            m_paramsLayout->addWidget(slider2);
            break;
        }

        case VisionNodeType::Threshold: {
            auto *chkOtsu = new QCheckBox("开启 Otsu 自动最适阈值求解", m_paramsContainer);
            chkOtsu->setChecked(node->param("otsu", true).toBool());
            connect(chkOtsu, &QCheckBox::toggled, this, [this, node](bool checked) {
                node->setParam("otsu", checked);
                executePipeline();
            });
            m_paramsLayout->addWidget(chkOtsu);
            break;
        }

        case VisionNodeType::ContourInspection: {
            double minA = node->param("minArea", 80).toDouble();
            auto *aLabel = new QLabel(QString("最小目标面积公差: %1 px²").arg(static_cast<int>(minA)), m_paramsContainer);
            aLabel->setObjectName("CardSubTitle");
            auto *slider = new QSlider(Qt::Horizontal, m_paramsContainer);
            slider->setRange(10, 1000);
            slider->setValue(static_cast<int>(minA));
            connect(slider, &QSlider::valueChanged, this, [this, node, aLabel](int v) {
                aLabel->setText(QString("最小目标面积公差: %1 px²").arg(v));
                node->setParam("minArea", v);
                executePipeline();
            });
            m_paramsLayout->addWidget(aLabel);
            m_paramsLayout->addWidget(slider);
            break;
        }

        default: {
            auto *info = new QLabel("该算子采用标准工业预设配置，随流自适应推演。", m_paramsContainer);
            info->setStyleSheet("color: #64748b; font-size: 12px;");
            m_paramsLayout->addWidget(info);
            break;
        }
    }

    // 刷新大图预览
    const cv::Mat &mat = node->outputMat();
    if (!mat.empty()) {
        cv::Mat displayMat;
        int pw = 330;
        int ph = 220;
        cv::resize(mat, displayMat, cv::Size(pw, ph), 0, 0, cv::INTER_LINEAR);

        QImage qimg;
        if (displayMat.channels() == 1) {
            qimg = QImage(displayMat.data, displayMat.cols, displayMat.rows, static_cast<int>(displayMat.step), QImage::Format_Grayscale8).copy();
        } else if (displayMat.channels() == 3) {
            cv::Mat rgb;
            cv::cvtColor(displayMat, rgb, cv::COLOR_BGR2RGB);
            qimg = QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
        }

        if (!qimg.isNull()) {
            m_previewImageLabel->setPixmap(QPixmap::fromImage(qimg));
        }

        m_previewMetricsLabel->setText(QString("📐 输出尺寸: %1x%2 | 通道: %3\n⏱️ 算子耗时: %4 ms | %5")
                                       .arg(mat.cols).arg(mat.rows).arg(mat.channels())
                                       .arg(node->lastLatencyMs(), 0, 'f', 2)
                                       .arg(node->statusMessage()));
    } else {
        m_previewImageLabel->setText("算子暂无输出图像\n点击顶部「运行」开始推演");
        m_previewMetricsLabel->setText("等待演算");
    }
}

void NodeGraphPipelinePage::onActivated() {
    if (m_nodes.isEmpty()) {
        loadPresetWorkflow();
    }
}

void NodeGraphPipelinePage::onDeactivated() {
    if (m_isStreaming) {
        toggleContinuousStream();
    }
}
