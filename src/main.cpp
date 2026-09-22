#include <QApplication>
#include "core/ToolManager.h"
#include "ui/MainWindow.h"
#include "modules/vision_matcher/VisionMatcherPage.h"
#include "modules/image_processor/ImageProcessorPage.h"
#include "modules/dev_toolbox/DevToolboxPage.h"

int main(int argc, char *argv[]) {
    // 启用现代化 UI 渲染特性
    QApplication app(argc, argv);
    app.setApplicationName("VisionCraft");
    app.setOrganizationName("VisionCraftStudio");

    // 1. 注册功能模块（模块化架构：增加新功能只需在这追加一行！）
    auto &mgr = ToolManager::instance();
    mgr.registerTool(new VisionMatcherPage());
    mgr.registerTool(new ImageProcessorPage());
    mgr.registerTool(new DevToolboxPage());

    // 2. 初始化主界面并显示
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
