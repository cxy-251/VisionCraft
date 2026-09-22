#include "HomePage.h"
#include "core/ToolManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <opencv2/core/version.hpp>

HomePage::HomePage(QWidget *parent) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(25);

    // 1. 顶部标题栏
    mainLayout->addWidget(createHeader());

    // 2. 中间卡片网格容器（带滚动条）
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto *scrollContent = new QWidget(scrollArea);
    m_cardGrid = new QGridLayout(scrollContent);
    m_cardGrid->setSpacing(20);
    m_cardGrid->setContentsMargins(0, 10, 0, 10);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // 3. 底部状态指示
    mainLayout->addWidget(createFooterInfo());

    // 加载所有已注册的工具卡片
    reloadTools();
}

QWidget* HomePage::createHeader() {
    auto *headerWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(headerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel("🚀 VisionCraft 开发者工坊", headerWidget);
    titleLabel->setStyleSheet("font-size: 26px; font-weight: 700; color: #1e293b;");

    auto *subtitleLabel = new QLabel(
        "基于现代 C++17、Qt 6 跨平台体系与 OpenCV 4 视觉中枢。请选择你要启动的工具模块：", 
        headerWidget
    );
    subtitleLabel->setStyleSheet("font-size: 14px; color: #64748b;");

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    return headerWidget;
}

QWidget* HomePage::createToolCard(const QString &id, const QString &icon, 
                                  const QString &title, const QString &desc) {
    auto *card = new QFrame(this);
    card->setObjectName("ToolCard");
    card->setMinimumSize(280, 170);
    card->setStyleSheet(
        "#ToolCard {"
        "   background-color: #ffffff;"
        "   border: 1px solid #e2e8f0;"
        "   border-radius: 12px;"
        "}"
        "#ToolCard:hover {"
        "   border: 1px solid #3b82f6;"
        "   background-color: #f8fafc;"
        "}"
    );

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(12);

    // 图标与标题行
    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(10);

    auto *iconLabel = new QLabel(icon, card);
    iconLabel->setStyleSheet("font-size: 24px;");

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");

    headerRow->addWidget(iconLabel);
    headerRow->addWidget(titleLabel);
    headerRow->addStretch();
    cardLayout->addLayout(headerRow);

    // 描述信息
    auto *descLabel = new QLabel(desc, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 13px; color: #64748b; line-height: 1.4;");
    cardLayout->addWidget(descLabel, 1);

    // 底部启动按钮
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *launchBtn = new QPushButton("进入工具 ➔", card);
    launchBtn->setCursor(Qt::PointingHandCursor);
    launchBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #2563eb;"
        "   color: #ffffff;"
        "   border: none;"
        "   border-radius: 6px;"
        "   padding: 6px 14px;"
        "   font-size: 13px;"
        "   font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1d4ed8;"
        "}"
    );

    connect(launchBtn, &QPushButton::clicked, this, [this, id]() {
        emit toolSelected(id);
    });

    btnRow->addWidget(launchBtn);
    cardLayout->addLayout(btnRow);

    return card;
}

QWidget* HomePage::createFooterInfo() {
    auto *footer = new QWidget(this);
    auto *layout = new QHBoxLayout(footer);
    layout->setContentsMargins(0, 0, 0, 0);

    QString infoText = QString("Qt %1 | OpenCV %2 | 运行环境: MSVC x64 | 跨平台架构")
                       .arg(QT_VERSION_STR)
                       .arg(CV_VERSION);
    auto *infoLabel = new QLabel(infoText, footer);
    infoLabel->setStyleSheet("color: #94a3b8; font-size: 12px;");

    auto *statusLabel = new QLabel("🟢 核心引擎在线就绪", footer);
    statusLabel->setStyleSheet("color: #16a34a; font-size: 12px; font-weight: 600;");

    layout->addWidget(infoLabel);
    layout->addStretch();
    layout->addWidget(statusLabel);
    return footer;
}

void HomePage::reloadTools() {
    // 清理旧卡片
    QLayoutItem *child;
    while ((child = m_cardGrid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    const auto &tools = ToolManager::instance().tools();
    int cols = 3;
    for (int i = 0; i < tools.size(); ++i) {
        auto *t = tools[i];
        int row = i / cols;
        int col = i % cols;
        auto *card = createToolCard(t->id(), t->icon(), t->title(), t->description());
        m_cardGrid->addWidget(card, row, col);
    }
}
