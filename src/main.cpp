#include <QApplication>
#include "core/ToolManager.h"
#include "ui/MainWindow.h"
#include "ui/KnowledgeExplorerPage.h"
#include "modules/IntegratedToolboxPage.h"
#include "modules/node_pipeline/NodeGraphPipelinePage.h"
#include "modules/batch_inspector/BatchInspectorPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 注册四大核心旗舰工作区 (知识图谱、多合一工具箱、节点图流水线、多核批质检中心)
    auto &mgr = ToolManager::instance();
    mgr.registerTool(new KnowledgeExplorerPage());
    mgr.registerTool(new IntegratedToolboxPage());
    mgr.registerTool(new NodeGraphPipelinePage());
    mgr.registerTool(new BatchInspectorPage());

    // 初始化主界面
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
