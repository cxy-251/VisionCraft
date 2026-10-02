#include "LegacyLab.h"

#include "core/ToolManager.h"
#include "modules/IntegratedToolboxPage.h"
#include "modules/f407_workbench/F407WorkbenchPage.h"
#include "ui/KnowledgeExplorerPage.h"
#include "ui/MainWindow.h"

LegacyLab::LegacyLab(QObject *parent)
    : QObject(parent)
{
}

LegacyLab::~LegacyLab()
{
    delete m_window.data();
}

void LegacyLab::open()
{
    if (!m_window) {
        // ToolManager 是旧代码的全局注册表，只能注册一次
        auto &mgr = ToolManager::instance();
        if (mgr.tools().isEmpty()) {
            mgr.registerTool(new KnowledgeExplorerPage());
            mgr.registerTool(new IntegratedToolboxPage());
            mgr.registerTool(new F407WorkbenchPage());
        }
        // 关闭窗口只是隐藏，不销毁：旧页面已挂到这个窗口下，而 ToolManager 还持有它们的指针
        m_window = new MainWindow();
        emit openedChanged();
    }
    m_window->show();
    m_window->raise();
    m_window->activateWindow();
}
