qt_add_executable(example_qt_crashdump main.cpp)
target_link_libraries(example_qt_crashdump PRIVATE Qt6::Core)
# 带调试信息、保留帧指针、导出符号：崩溃时的调用栈才能还原成函数名和行号
target_compile_options(example_qt_crashdump PRIVATE -g -O1 -fno-omit-frame-pointer -fno-inline)
target_link_options(example_qt_crashdump PRIVATE -rdynamic)
