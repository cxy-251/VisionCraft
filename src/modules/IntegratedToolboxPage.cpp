#include "IntegratedToolboxPage.h"
#include "vision_matcher/VisionMatcherPage.h"
#include "image_processor/ImageProcessorPage.h"
#include "dev_toolbox/DevToolboxPage.h"
#include <QVBoxLayout>

IntegratedToolboxPage::IntegratedToolboxPage(QWidget *parent) : IToolPage(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 10, 15, 15);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #334155; border-radius: 8px; background: transparent; }"
        "QTabBar::tab { font-size: 13px; font-weight: 600; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 4px; }"
        "QTabBar::tab:selected { background-color: #2563eb; color: #ffffff; }"
        "QTabBar::tab:!selected { background-color: #1e293b; color: #94a3b8; }"
    );

    m_matcherPage = new VisionMatcherPage(this);
    m_processorPage = new ImageProcessorPage(this);
    m_devPage = new DevToolboxPage(this);

    m_tabWidget->addTab(m_matcherPage, "🎯 屏幕找图与模板匹配");
    m_tabWidget->addTab(m_processorPage, "🎨 OpenCV 图像处理工坊");
    m_tabWidget->addTab(m_devPage, "🛠️ 系统与 JSON 实用工具");

    layout->addWidget(m_tabWidget);
}

void IntegratedToolboxPage::onActivated() {
    int cur = m_tabWidget->currentIndex();
    if (cur == 0) m_matcherPage->onActivated();
    else if (cur == 1) m_processorPage->onActivated();
    else if (cur == 2) m_devPage->onActivated();
}

void IntegratedToolboxPage::onDeactivated() {
    m_matcherPage->onDeactivated();
    m_processorPage->onDeactivated();
    m_devPage->onDeactivated();
}
