#include "IntegratedToolboxPage.h"
#include "live_pipeline/LiveScreenPipelinePage.h"
#include "vision_matcher/VisionMatcherPage.h"
#include "image_processor/ImageProcessorPage.h"
#include "dev_toolbox/DevToolboxPage.h"
#include "ThemeManager.h"
#include <QVBoxLayout>

IntegratedToolboxPage::IntegratedToolboxPage(QWidget *parent) : IToolPage(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 15);

    m_tabWidget = new QTabWidget(this);
    auto updateTabsStyle = [this](bool isDark) {
        if (isDark) {
            m_tabWidget->setStyleSheet(
                "QTabWidget::pane { border: 1px solid #334155; border-radius: 8px; background: transparent; }"
                "QTabBar::tab { font-size: 13px; font-weight: 600; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 4px; }"
                "QTabBar::tab:selected { background-color: #2563eb; color: #ffffff; }"
                "QTabBar::tab:!selected { background-color: #1e293b; color: #94a3b8; }"
            );
        } else {
            m_tabWidget->setStyleSheet(
                "QTabWidget::pane { border: 1px solid #cbd5e1; border-radius: 8px; background: transparent; }"
                "QTabBar::tab { font-size: 13px; font-weight: 600; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 4px; }"
                "QTabBar::tab:selected { background-color: #2563eb; color: #ffffff; }"
                "QTabBar::tab:!selected { background-color: #e2e8f0; color: #475569; }"
            );
        }
    };
    updateTabsStyle(ThemeManager::instance().isDarkMode());
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, updateTabsStyle);

    m_livePipelinePage = new LiveScreenPipelinePage(this);
    m_matcherPage = new VisionMatcherPage(this);
    m_processorPage = new ImageProcessorPage(this);
    m_devPage = new DevToolboxPage(this);

    m_tabWidget->addTab(m_livePipelinePage, "⚡ 实时屏幕流·算子流水线");
    m_tabWidget->addTab(m_matcherPage, "🎯 屏幕找图与模板匹配");
    m_tabWidget->addTab(m_processorPage, "🎨 OpenCV 图像处理工坊");
    m_tabWidget->addTab(m_devPage, "🛠️ 系统与 JSON 实用工具");

    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int idx) {
        if (idx == 0) m_livePipelinePage->onActivated();
        else m_livePipelinePage->onDeactivated();

        if (idx == 1) m_matcherPage->onActivated();
        else if (idx == 2) m_processorPage->onActivated();
        else if (idx == 3) m_devPage->onActivated();
    });

    layout->addWidget(m_tabWidget);
}

void IntegratedToolboxPage::onActivated() {
    int cur = m_tabWidget->currentIndex();
    if (cur == 0) m_livePipelinePage->onActivated();
    else if (cur == 1) m_matcherPage->onActivated();
    else if (cur == 2) m_processorPage->onActivated();
    else if (cur == 3) m_devPage->onActivated();
}

void IntegratedToolboxPage::onDeactivated() {
    m_livePipelinePage->onDeactivated();
    m_matcherPage->onDeactivated();
    m_processorPage->onDeactivated();
    m_devPage->onDeactivated();
}
