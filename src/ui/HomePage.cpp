#include "HomePage.h"
#include "ThemeManager.h"
#include "core/ToolManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <opencv2/core/version.hpp>

HomePage::HomePage(QWidget *parent) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(50, 40, 50, 40);
    mainLayout->setSpacing(30);

    // 1. 顶部标语
    mainLayout->addWidget(createHeader());

    // 2. 中间卡片容器（居中对齐，杜绝截断）
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto *scrollContent = new QWidget(scrollArea);
    m_cardGrid = new QGridLayout(scrollContent);
    m_cardGrid->setSpacing(24);
    m_cardGrid->setContentsMargins(0, 10, 0, 10);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // 3. 底部状态栏
    mainLayout->addWidget(createFooterInfo());

    reloadTools();
}

QWidget* HomePage::createHeader() {
    auto *headerWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(headerWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel("🚀 VisionCraft 交互知识与开发工坊", headerWidget);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 800;");

    auto *subtitleLabel = new QLabel(
        "数据驱动的 Qt 6 与 OpenCV 4 架构级全景实验室。请选择进入的核心板块：", 
        headerWidget
    );
    subtitleLabel->setStyleSheet("font-size: 14px; opacity: 0.8;");

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    return headerWidget;
}

QWidget* HomePage::createToolCard(const QString &id, const QString &icon, 
                                  const QString &title, const QString &desc) {
    auto *card = new QFrame(this);
    card->setObjectName("ToolCard");
    card->setMinimumSize(320, 200); // 充分高度，彻底消灭文字截断
    card->setStyleSheet(ThemeManager::instance().cardStyleSheet());

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(14);

    // 顶部：大图标与标题
    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(12);

    auto *iconLabel = new QLabel(icon, card);
    iconLabel->setStyleSheet("font-size: 32px;");

    auto *titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 700;");
    titleLabel->setWordWrap(true);

    headerRow->addWidget(iconLabel);
    headerRow->addWidget(titleLabel, 1);
    cardLayout->addLayout(headerRow);

    // 描述信息（自适应折行）
    auto *descLabel = new QLabel(desc, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 13px; line-height: 1.6; opacity: 0.85;");
    cardLayout->addWidget(descLabel, 1);

    // 底部启动按钮
    auto *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto *launchBtn = new QPushButton("开启探索 ➔", card);
    launchBtn->setCursor(Qt::PointingHandCursor);
    launchBtn->setStyleSheet(
        "QPushButton { padding: 8px 18px; font-size: 13px; font-weight: bold; border-radius: 6px; }"
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

    QString infoText = QString("Qt %1 | OpenCV %2 | MSVC x64 | 跨平台纯净架构")
                       .arg(QT_VERSION_STR)
                       .arg(CV_VERSION);
    auto *infoLabel = new QLabel(infoText, footer);
    infoLabel->setStyleSheet("font-size: 12px; opacity: 0.7;");

    auto *statusLabel = new QLabel("🟢 系统内核在线", footer);
    statusLabel->setStyleSheet("color: #10b981; font-size: 12px; font-weight: bold;");

    layout->addWidget(infoLabel);
    layout->addStretch();
    layout->addWidget(statusLabel);
    return footer;
}

void HomePage::reloadTools() {
    QLayoutItem *child;
    while ((child = m_cardGrid->takeAt(0)) != nullptr) {
        if (child->widget()) delete child->widget();
        delete child;
    }

    const auto &tools = ToolManager::instance().tools();
    int cols = 2; // 两个旗舰大卡片并排，视觉平衡感绝佳
    for (int i = 0; i < tools.size(); ++i) {
        auto *t = tools[i];
        int row = i / cols;
        int col = i % cols;
        auto *card = createToolCard(t->id(), t->icon(), t->title(), t->description());
        m_cardGrid->addWidget(card, row, col);
    }
}
