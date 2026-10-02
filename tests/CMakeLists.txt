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
