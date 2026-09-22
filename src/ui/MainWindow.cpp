#include "MainWindow.h"
#include "ThemeManager.h"
#include "core/ToolManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setupUI();
    applyTheme(ThemeManager::instance().isDarkMode());

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
}

void MainWindow::setupUI() {
    setWindowTitle("VisionCraft - Qt 6 & OpenCV 4 全景交互实验室");
    resize(1220, 800);
    setMinimumSize(980, 640);

    auto *centralWidget = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 1. 顶部全局导航栏
    m_topBar = new QWidget(centralWidget);
    m_topBar->setFixedHeight(58);
    m_topBar->setObjectName("TopBar");

    auto *topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(24, 0, 24, 0);
    topLayout->setSpacing(16);

    auto *logoLabel = new QLabel("✨ <b>VisionCraft</b>", m_topBar);
    logoLabel->setStyleSheet("font-size: 18px; font-weight: 800; border: none;");
    topLayout->addWidget(logoLabel);

    m_breadcrumbLabel = new QLabel(" | 🏠 首页仪表盘", m_topBar);
    m_breadcrumbLabel->setStyleSheet("font-size: 14px; border: none;");
    topLayout->addWidget(m_breadcrumbLabel);

    topLayout->addStretch();

    // 主题切换按钮
    m_themeToggleBtn = new QPushButton(m_topBar);
    m_themeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_themeToggleBtn->setStyleSheet("QPushButton { font-size: 12px; padding: 6px 12px; }");
    connect(m_themeToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    topLayout->addWidget(m_themeToggleBtn);

    // “返回首页”按钮（处于首页时隐藏）
    m_backHomeBtn = new QPushButton("⬅ 返回功能首页", m_topBar);
    m_backHomeBtn->setCursor(Qt::PointingHandCursor);
    m_backHomeBtn->setVisible(false);
    connect(m_backHomeBtn, &QPushButton::clicked, this, &MainWindow::navigateToHome);
    topLayout->addWidget(m_backHomeBtn);

    rootLayout->addWidget(m_topBar);

    // 2. 页面容器 QStackedWidget
    m_stackWidget = new QStackedWidget(centralWidget);
    m_homePage = new HomePage(m_stackWidget);
    m_stackWidget->addWidget(m_homePage);

    // 监听首页卡片点击
    connect(m_homePage, &HomePage::toolSelected, this, &MainWindow::navigateToTool);

    // 将注册中心中的所有工具页面挂载到 Stack 中
    const auto &tools = ToolManager::instance().tools();
    for (auto *tool : tools) {
        m_stackWidget->addWidget(tool);
    }

    rootLayout->addWidget(m_stackWidget, 1);
    setCentralWidget(centralWidget);
}

void MainWindow::applyTheme(bool isDark) {
    qApp->setStyleSheet(ThemeManager::instance().currentGlobalStyleSheet());

    if (isDark) {
        m_topBar->setStyleSheet("background-color: #1e293b; border-bottom: 1px solid #334155;");
        m_breadcrumbLabel->setStyleSheet("font-size: 14px; color: #94a3b8; border: none;");
        m_themeToggleBtn->setText("🌙 深色 (系统) ➔ 切浅色");
    } else {
        m_topBar->setStyleSheet("background-color: #ffffff; border-bottom: 1px solid #e2e8f0;");
        m_breadcrumbLabel->setStyleSheet("font-size: 14px; color: #64748b; border: none;");
        m_themeToggleBtn->setText("☀️ 浅色 (系统) ➔ 切深色");
    }

    m_homePage->reloadTools();
}

void MainWindow::toggleTheme() {
    auto &tm = ThemeManager::instance();
    if (tm.isDarkMode()) {
        tm.setThemeMode(ThemeManager::Light);
    } else {
        tm.setThemeMode(ThemeManager::Dark);
    }
}

void MainWindow::navigateToHome() {
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
