cmake_minimum_required(VERSION 3.16)
project(VisionCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. Qt 6
list(APPEND CMAKE_PREFIX_PATH "C:/02Programmer/Qt/6.7.3/msvc2019_64")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# 2. OpenCV 4
set(OpenCV_DIR "C:/02Programmer/opencv/build")
find_package(OpenCV REQUIRED)

# 3. vcpkg: fmt & nlohmann_json
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 头文件搜索路径
include_directories(
    src
    src/ui
    ${OpenCV_INCLUDE_DIRS}
)

# 源码清单
set(SOURCES
    src/main.cpp
    src/core/IToolPage.h
    src/core/ToolManager.h
    src/core/ToolManager.cpp
    src/core/ScreenCapture.h
    src/core/ScreenCapture.cpp
    src/core/KnowledgeModel.h
    src/core/KnowledgeRegistry.h
    src/core/KnowledgeRegistry.cpp
    src/ui/ThemeManager.h
    src/ui/ThemeManager.cpp
    src/ui/HomePage.h
    src/ui/HomePage.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/KnowledgeExplorerPage.h
    src/ui/KnowledgeExplorerPage.cpp
    src/modules/IntegratedToolboxPage.h
    src/modules/IntegratedToolboxPage.cpp
    src/modules/live_pipeline/LiveScreenPipelinePage.h
    src/modules/live_pipeline/LiveScreenPipelinePage.cpp
    src/modules/vision_matcher/VisionMatcherPage.h
    src/modules/vision_matcher/VisionMatcherPage.cpp
    src/modules/image_processor/ImageProcessorPage.h
    src/modules/image_processor/ImageProcessorPage.cpp
    src/modules/dev_toolbox/DevToolboxPage.h
    src/modules/dev_toolbox/DevToolboxPage.cpp
    src/modules/node_pipeline/ConnectionWireItem.h
    src/modules/node_pipeline/ConnectionWireItem.cpp
    src/modules/node_pipeline/VisionNodeItem.h
    src/modules/node_pipeline/VisionNodeItem.cpp
    src/modules/node_pipeline/NodeGraphPipelinePage.h
    src/modules/node_pipeline/NodeGraphPipelinePage.cpp
    src/modules/batch_inspector/BatchInspectorPage.h
    src/modules/batch_inspector/BatchInspectorPage.cpp
)

add_executable(VisionCraft WIN32 ${SOURCES})

target_link_libraries(VisionCraft PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    ${OpenCV_LIBS}
    fmt::fmt
    nlohmann_json::nlohmann_json
)

# 确保 Qt6Concurrent 动态链接库始终自动部署到可执行文件同级目录
add_custom_command(TARGET VisionCraft POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "C:/02Programmer/Qt/6.7.3/msvc2019_64/bin/Qt6Concurrent.dll"
    "$<TARGET_FILE_DIR:VisionCraft>"
)

