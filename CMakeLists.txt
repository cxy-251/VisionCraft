cmake_minimum_required(VERSION 3.21)
project(VisionCraft LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. Qt 6
if(WIN32 AND EXISTS "C:/02Programmer/Qt/6.7.3/msvc2019_64")
    list(APPEND CMAKE_PREFIX_PATH "C:/02Programmer/Qt/6.7.3/msvc2019_64")
endif()
find_package(Qt6 6.7 REQUIRED COMPONENTS Core Gui Widgets Concurrent SerialPort Network Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.7)

# 2. OpenCV 4
if(WIN32 AND EXISTS "C:/02Programmer/opencv/build/x64/vc16/lib")
    set(OpenCV_DIR "C:/02Programmer/opencv/build/x64/vc16/lib")
elseif(WIN32 AND EXISTS "C:/02Programmer/opencv/build")
    set(OpenCV_DIR "C:/02Programmer/opencv/build")
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs features2d calib3d objdetect video)

# 3. Dependencies: fmt & nlohmann_json
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

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
    src/core/KnowledgeRegistry_OpenCV_Vision.cpp
    src/core/KnowledgeRegistry_OpenCV_Industrial.cpp
    src/core/KnowledgeRegistry_Qt_Core.cpp
    src/core/KnowledgeRegistry_Qt_Architecture.cpp
    src/core/KnowledgeRegistry_Linux_Environment.cpp
    src/core/KnowledgeRegistry_F407ZG.cpp
    src/ui/CppSyntaxHighlighter.h
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
    src/modules/f407_workbench/F407WorkbenchPage.h
    src/modules/f407_workbench/F407WorkbenchPage.cpp
)

if(WIN32)
    qt_add_executable(VisionCraft WIN32 ${SOURCES})
else()
    qt_add_executable(VisionCraft ${SOURCES})
endif()

# ============================================================
# 新版界面（QML 外壳 + 手册）。旧 Widgets 代码通过「实验室」入口打开，迁移完成后删除。
# ============================================================
# Qt 6.8 起：给 qml/ 下每个子目录也生成 qmldir（6.7 没有这条策略）
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()
# [region qml-module]
# 单例标记必须在 qt_add_qml_module 之前设置
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(VisionCraft
    URI VisionCraft
    # 默认输出到 build/VisionCraft/，会和同名的可执行文件冲突
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/qml/VisionCraft
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/shell/NavRail.qml
        qml/shell/LabPage.qml
        qml/device/DevicePage.qml
        qml/station/StationPage.qml
        qml/data/DataPage.qml
        qml/data/TimeChart.qml
        qml/handbook/HandbookPage.qml
        qml/handbook/HandbookIndex.qml
        qml/handbook/Volume.qml
        qml/handbook/Chapter.qml
        qml/handbook/Entry.qml
        qml/handbook/Section.qml
        qml/handbook/Para.qml
        qml/handbook/Why.qml
        qml/handbook/KeyPoints.qml
        qml/handbook/CodeRef.qml
        qml/handbook/Demo.qml
        qml/handbook/Pitfall.qml
        qml/handbook/Try.qml
        qml/handbook/InSystem.qml
        qml/handbook/Block.qml
        qml/demos/qt/SignalTraceView.qml
        qml/demos/qt/DeadlockView.qml
        qml/demos/system/FrameDemoView.qml
        qml/demos/opencv/EvalView.qml
        qml/demos/opencv/MatView.qml
    SOURCES
        src/handbook/SourceProvider.h
        src/handbook/SourceProvider.cpp
        src/handbook/CodeHighlighter.h
        src/handbook/CodeHighlighter.cpp
        src/legacy/LegacyLab.h
        src/legacy/LegacyLab.cpp
        src/demos/qt/SignalTraceDemo.h
        src/demos/qt/SignalTraceDemo.cpp
        src/demos/qt/DeadlockDemo.h
        src/demos/qt/DeadlockDemo.cpp
        src/demos/system/FrameDemo.h
        src/demos/system/FrameDemo.cpp
        src/demos/opencv/EvalDemo.h
        src/demos/opencv/EvalDemo.cpp
        src/demos/opencv/MatDemo.h
        src/demos/opencv/MatDemo.cpp
        src/device/DeviceLink.h
        src/device/DeviceLink.cpp
        src/app/ImageView.h
        src/app/ImageView.cpp
        src/app/StationController.h
        src/app/StationController.cpp
)
# [endregion]

# 视觉：模拟产线与检测算法
target_sources(VisionCraft PRIVATE
    src/vision/PartGenerator.h
    src/vision/PartGenerator.cpp
    src/vision/Inspector.h
    src/vision/Inspector.cpp
)

# 设备通信：协议（与固件共用的 C 代码）+ 通道
target_sources(VisionCraft PRIVATE
    protocol/vc_protocol.h
    protocol/vc_protocol.c
    src/device/Transport.h
    src/device/SimTransport.h
    src/device/SimTransport.cpp
    src/device/SerialTransport.h
    src/device/SerialTransport.cpp
    src/device/RttTransport.h
    src/device/RttTransport.cpp
)

# 手册正文、示例代码与被引用的源码：打包进资源，CodeRef 运行时按文件路径读取
file(GLOB_RECURSE VC_HANDBOOK_FILES CONFIGURE_DEPENDS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    handbook/*.qml
    examples/*.cpp examples/*.h examples/*.c examples/*.txt
    src/*.cpp src/*.h
    protocol/*.c protocol/*.h
    firmware/station/App/*.c firmware/station/App/*.h
    firmware/station/Core/Src/*.c firmware/station/Core/Inc/*.h
    firmware/station/station.ioc
    tests/*.cpp
    tools/*.cpp tools/*.sh tools/*.tcl
    CMakeLists.txt qml/*.qml)
qt_add_resources(VisionCraft "vc_handbook_content" PREFIX "/" FILES ${VC_HANDBOOK_FILES})

target_compile_definitions(VisionCraft PRIVATE VC_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
# [region registration-includes]
# qmltyperegistrar 生成的注册代码按文件名 #include 头文件，这些目录必须在搜索路径里
target_include_directories(VisionCraft PRIVATE src/handbook src/legacy src/demos/qt src/demos/system src/demos/opencv src/device src/app src/vision protocol)
# [endregion]

target_link_libraries(VisionCraft PRIVATE
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::SerialPort
    ${OpenCV_LIBS}
    fmt::fmt
    nlohmann_json::nlohmann_json
)

# 确保 Windows 下 Qt6Concurrent 动态链接库始终自动部署到可执行文件同级目录
if(WIN32 AND EXISTS "C:/02Programmer/Qt/6.7.3/msvc2019_64/bin/Qt6Concurrent.dll")
    add_custom_command(TARGET VisionCraft POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/02Programmer/Qt/6.7.3/msvc2019_64/bin/Qt6Concurrent.dll"
        "$<TARGET_FILE_DIR:VisionCraft>"
    )
endif()


# 手册引用的独立示例程序
option(VC_BUILD_EXAMPLES "Build handbook examples" ON)
if(VC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# 测试（ctest --test-dir build）
option(VC_BUILD_TESTS "Build tests" ON)
if(VC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 开发工具（vclink_probe：命令行验证与板子的通信）
add_subdirectory(tools)
