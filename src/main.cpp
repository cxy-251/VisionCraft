#include <QApplication>
#include "core/ToolManager.h"
#include "ui/MainWindow.h"
#include "ui/KnowledgeExplorerPage.h"
#include "modules/vision_matcher/VisionMatcherPage.h"
#include "modules/image_processor/ImageProcessorPage.h"
#include "modules/dev_toolbox/DevToolboxPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 注册各功能模块（旗舰知识实验室排在第一位！）
    auto &mgr = ToolManager::instance();
    mgr.registerTool(new KnowledgeExplorerPage());
    mgr.registerTool(new VisionMatcherPage());
    mgr.registerTool(new ImageProcessorPage());
    mgr.registerTool(new DevToolboxPage());

    // 初始化主界面
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
