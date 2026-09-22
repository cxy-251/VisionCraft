cmake_minimum_required(VERSION 3.16)
project(VisionCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 1. Qt 6 跨平台 GUI
list(APPEND CMAKE_PREFIX_PATH "C:/02Programmer/Qt/6.7.3/msvc2019_64")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# 2. OpenCV 4 计算机视觉
set(OpenCV_DIR "C:/02Programmer/opencv/build")
find_package(OpenCV REQUIRED)

# 3. vcpkg 依赖库：fmt & nlohmann_json
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 头文件搜索路径
include_directories(
    src
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
    src/ui/HomePage.h
    src/ui/HomePage.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/modules/vision_matcher/VisionMatcherPage.h
    src/modules/vision_matcher/VisionMatcherPage.cpp
    src/modules/image_processor/ImageProcessorPage.h
    src/modules/image_processor/ImageProcessorPage.cpp
    src/modules/dev_toolbox/DevToolboxPage.h
    src/modules/dev_toolbox/DevToolboxPage.cpp
)

add_executable(VisionCraft WIN32 ${SOURCES})

target_link_libraries(VisionCraft PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OpenCV_LIBS}
    fmt::fmt
    nlohmann_json::nlohmann_json
)
