#include <QApplication>
#include "core/ToolManager.h"
#include "ui/MainWindow.h"
#include "ui/KnowledgeExplorerPage.h"
#include "modules/IntegratedToolboxPage.h"
#include "modules/f407_workbench/F407WorkbenchPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 注册核心版块（知识图谱、综合工具箱、F407 硬件工作台）
    auto &mgr = ToolManager::instance();
    mgr.registerTool(new KnowledgeExplorerPage());
    mgr.registerTool(new IntegratedToolboxPage());
    mgr.registerTool(new F407WorkbenchPage());

    // 初始化主界面
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
