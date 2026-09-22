#include "MainWindow.h"
#include "core/ToolManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    setupGlobalStyle();
}

void MainWindow::setupUI() {
    setWindowTitle("VisionCraft 现代化跨平台视觉工作台");
    resize(1180, 760);
    setMinimumSize(960, 600);

    auto *centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. 顶部全局导航栏
    m_topBar = new QWidget(centralWidget);
    m_topBar->setFixedHeight(56);
    m_topBar->setStyleSheet(
        "background-color: #ffffff; border-bottom: 1px solid #e2e8f0;"
    );

    auto *topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(24, 0, 24, 0);
    topLayout->setSpacing(16);

    auto *logoLabel = new QLabel("✨ <b>VisionCraft</b>", m_topBar);
    logoLabel->setStyleSheet("font-size: 18px; color: #1e293b; border: none;");
    topLayout->addWidget(logoLabel);

    m_breadcrumbLabel = new QLabel(" | 🏠 首页仪表盘", m_topBar);
    m_breadcrumbLabel->setStyleSheet("font-size: 14px; color: #64748b; border: none;");
    topLayout->addWidget(m_breadcrumbLabel);

    topLayout->addStretch();

    // “返回首页”按钮（处于首页时隐藏）
    m_backHomeBtn = new QPushButton("⬅ 返回功能首页", m_topBar);
    m_backHomeBtn->setCursor(Qt::PointingHandCursor);
    m_backHomeBtn->setVisible(false);
    m_backHomeBtn->setStyleSheet(
        "QPushButton {"
        "   background-color: #f1f5f9;"
        "   color: #1e293b;"
        "   border: 1px solid #cbd5e1;"
        "   border-radius: 6px;"
        "   padding: 6px 14px;"
        "   font-weight: 500;"
        "   font-size: 13px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #e2e8f0;"
        "}"
    );
    connect(m_backHomeBtn, &QPushButton::clicked, this, &MainWindow::navigateToHome);
    topLayout->addWidget(m_backHomeBtn);

    rootLayout->addWidget(m_topBar);

    // 2. 页面容器 QStackedWidget
    m_stackWidget = new QStackedWidget(centralWidget);
    m_homePage = new HomePage(m_stackWidget);
    m_stackWidget->addWidget(m_homePage);

    // 监听首页的卡片点击
    connect(m_homePage, &HomePage::toolSelected, this, &MainWindow::navigateToTool);

    // 将注册中心中的所有工具页面挂载到 Stack 中
    const auto &tools = ToolManager::instance().tools();
    for (auto *tool : tools) {
        m_stackWidget->addWidget(tool);
    }

    rootLayout->addWidget(m_stackWidget, 1);
    setCentralWidget(centralWidget);
}

void MainWindow::setupGlobalStyle() {
    // 现代浅灰主背景
    setStyleSheet(
        "QMainWindow { background-color: #f8fafc; }"
        "QWidget { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', 'Microsoft YaHei', sans-serif; }"
    );
}

void MainWindow::navigateToHome() {
    // 触发当前正在离开的工具的 onDeactivated
    auto *currentTool = qobject_cast<IToolPage*>(m_stackWidget->currentWidget());
    if (currentTool) {
        currentTool->onDeactivated();
    }

    m_stackWidget->setCurrentWidget(m_homePage);
    m_backHomeBtn->setVisible(false);
    m_breadcrumbLabel->setText(" | 🏠 首页仪表盘");
}

void MainWindow::navigateToTool(const QString &toolId) {
    auto *tool = ToolManager::instance().findTool(toolId);
    if (!tool) return;

    m_stackWidget->setCurrentWidget(tool);
    tool->onActivated();

    m_backHomeBtn->setVisible(true);
    m_breadcrumbLabel->setText(QString(" | %1 %2").arg(tool->icon(), tool->title()));
}
