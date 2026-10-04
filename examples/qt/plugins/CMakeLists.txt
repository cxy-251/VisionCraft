# 主程序
qt_add_executable(example_qt_plugins main.cpp operator.h)
target_link_libraries(example_qt_plugins PRIVATE Qt6::Core)

# 插件：MODULE 库，只能被 dlopen 加载，不能被链接。放到主程序旁边的 plugins/ 目录
set(VC_PLUGIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/plugins)
foreach(variant IN ITEMS v1 v2)
    add_library(example_qt_plugin_invert_${variant} MODULE invert.cpp invert.json operator.h)
    target_link_libraries(example_qt_plugin_invert_${variant} PRIVATE Qt6::Core)
    target_compile_definitions(example_qt_plugin_invert_${variant} PRIVATE VC_PLUGIN_VERSION="${variant}")
    set_target_properties(example_qt_plugin_invert_${variant} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${VC_PLUGIN_DIR}/${variant} OUTPUT_NAME invert)
    add_dependencies(example_qt_plugins example_qt_plugin_invert_${variant})
endforeach()
foreach(name IN ITEMS threshold_old mislabeled)
    add_library(example_qt_plugin_${name} MODULE ${name}.cpp operator.h)
    target_link_libraries(example_qt_plugin_${name} PRIVATE Qt6::Core)
    set_target_properties(example_qt_plugin_${name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${VC_PLUGIN_DIR}/v1 OUTPUT_NAME ${name})
    add_dependencies(example_qt_plugins example_qt_plugin_${name})
endforeach()
