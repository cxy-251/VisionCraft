qt_add_executable(example_qt_i18n main.cpp)
target_link_libraries(example_qt_i18n PRIVATE Qt6::Widgets)

# 把翻译源文件 .ts 编译成程序运行时加载的 .qm（lrelease 来自 Qt 的 linguist 工具）
find_program(VC_LRELEASE lrelease HINTS /usr/lib/qt6/bin)
if(VC_LRELEASE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/i18n_en.qm
        COMMAND ${VC_LRELEASE} ${CMAKE_CURRENT_SOURCE_DIR}/i18n_en.ts -qm ${CMAKE_CURRENT_BINARY_DIR}/i18n_en.qm
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/i18n_en.ts)
    add_custom_target(example_qt_i18n_qm DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/i18n_en.qm)
    add_dependencies(example_qt_i18n example_qt_i18n_qm)
else()
    message(STATUS "没有找到 lrelease：example_qt_i18n 运行时找不到英文翻译")
endif()
