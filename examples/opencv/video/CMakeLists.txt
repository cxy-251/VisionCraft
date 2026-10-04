# 视频读写要 videoio 模块，主程序不需要它，所以只在这个示例里单独找
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio video)
qt_add_executable(example_opencv_video main.cpp)
target_include_directories(example_opencv_video PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_video PRIVATE Qt6::Core opencv_core opencv_imgproc opencv_videoio opencv_video)
