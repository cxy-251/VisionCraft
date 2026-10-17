qt_add_executable(example_qml_components ../lesson_main.cpp)
target_link_libraries(example_qml_components PRIVATE Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(example_qml_components PRIVATE EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
