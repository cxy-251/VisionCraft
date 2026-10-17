find_package(Qt6 REQUIRED COMPONENTS Quick3D)
qt_add_executable(example_spatial_lessons main.cpp)
qt_add_qml_module(example_spatial_lessons URI SpatialLessons
    QML_FILES Main.qml SOURCES ProjectionBridge.h)
target_include_directories(example_spatial_lessons PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(example_spatial_lessons PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Quick3D vc_teaching_vision)
