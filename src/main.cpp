#include <QApplication>
#include "core/ToolManager.h"
#include "ui/MainWindow.h"
#include "ui/KnowledgeExplorerPage.h"
#include "modules/IntegratedToolboxPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 注册两大核心版块（知识图谱与互动实验室 + 综合工具箱）
    auto &mgr = ToolManager::instance();
    mgr.registerTool(new KnowledgeExplorerPage());
    mgr.registerTool(new IntegratedToolboxPage());

    // 初始化主界面
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
