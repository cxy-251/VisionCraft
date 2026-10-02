find_package(Qt6 REQUIRED COMPONENTS Test)

# 「信号与槽」一节的两个演示：没有人点按钮时，用测试确认它们的行为和手册里写的一致
qt_add_executable(tst_qt_demos
    tst_qt_demos.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/qt/SignalTraceDemo.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/qt/DeadlockDemo.cpp
)
target_include_directories(tst_qt_demos PRIVATE ${PROJECT_SOURCE_DIR}/src/demos/qt)
target_link_libraries(tst_qt_demos PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
add_test(NAME tst_qt_demos COMMAND tst_qt_demos)

# 协议编解码（与固件共用的 C 代码）
qt_add_executable(tst_protocol tst_protocol.cpp ${PROJECT_SOURCE_DIR}/protocol/vc_protocol.c)
target_include_directories(tst_protocol PRIVATE ${PROJECT_SOURCE_DIR}/protocol)
target_link_libraries(tst_protocol PRIVATE Qt6::Core Qt6::Test)
add_test(NAME tst_protocol COMMAND tst_protocol)

# DeviceLink + 模拟器：整条上位机链路（组帧、配对、超时、事件、遥测、吞吐）
qt_add_executable(tst_devicelink tst_devicelink.cpp
    ${PROJECT_SOURCE_DIR}/protocol/vc_protocol.c
    ${PROJECT_SOURCE_DIR}/src/device/DeviceLink.cpp
    ${PROJECT_SOURCE_DIR}/src/device/SimTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/SerialTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/RttTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/Transport.h
)
target_include_directories(tst_devicelink PRIVATE ${PROJECT_SOURCE_DIR}/protocol ${PROJECT_SOURCE_DIR}/src/device)
target_link_libraries(tst_devicelink PRIVATE Qt6::Core Qt6::Network Qt6::SerialPort Qt6::Qml Qt6::Test)
add_test(NAME tst_devicelink COMMAND tst_devicelink)

# 检测算法：用模拟产线的标准答案算混淆矩阵
qt_add_executable(tst_inspector tst_inspector.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/Inspector.cpp
)
target_include_directories(tst_inspector PRIVATE ${PROJECT_SOURCE_DIR}/src/vision ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tst_inspector PRIVATE Qt6::Core Qt6::Test ${OpenCV_LIBS})
add_test(NAME tst_inspector COMMAND tst_inspector)

# 手册「二进制协议」一节的演示
qt_add_executable(tst_frame_demo tst_frame_demo.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/system/FrameDemo.cpp
    ${PROJECT_SOURCE_DIR}/protocol/vc_protocol.c)
target_include_directories(tst_frame_demo PRIVATE ${PROJECT_SOURCE_DIR}/src/demos/system ${PROJECT_SOURCE_DIR}/protocol)
target_link_libraries(tst_frame_demo PRIVATE Qt6::Core Qt6::Qml Qt6::Test)
add_test(NAME tst_frame_demo COMMAND tst_frame_demo)

# 手册「cv::Mat 内存模型」的演示
qt_add_executable(tst_mat_demo tst_mat_demo.cpp ${PROJECT_SOURCE_DIR}/src/demos/opencv/MatDemo.cpp)
target_include_directories(tst_mat_demo PRIVATE ${PROJECT_SOURCE_DIR}/src/demos/opencv ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tst_mat_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test ${OpenCV_LIBS})
add_test(NAME tst_mat_demo COMMAND tst_mat_demo)

# 手册「全局阈值与 OTSU」的演示
qt_add_executable(tst_threshold_demo tst_threshold_demo.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/opencv/ThresholdDemo.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp)
target_include_directories(tst_threshold_demo PRIVATE ${PROJECT_SOURCE_DIR}/src/demos/opencv ${PROJECT_SOURCE_DIR}/src/vision
                           ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tst_threshold_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test ${OpenCV_LIBS})
add_test(NAME tst_threshold_demo COMMAND tst_threshold_demo)
