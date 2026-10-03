qt_add_executable(example_qml_bindings main.cpp)
target_link_libraries(example_qml_bindings PRIVATE Qt6::Qml)
# QML 文件不编译进程序，运行时从源码目录读取，方便改了直接再跑
target_compile_definitions(example_qml_bindings PRIVATE EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
