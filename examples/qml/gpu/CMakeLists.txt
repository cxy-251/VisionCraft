find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
qt_add_executable(example_gpu_lessons main.cpp)
qt_add_qml_module(example_gpu_lessons URI GpuLessons QML_FILES Main.qml)
target_link_libraries(example_gpu_lessons PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia)
# [region shaders]
qt_add_shaders(example_gpu_lessons lesson_shaders
    PREFIX "/shaders" BASE shaders
    FILES shaders/color.frag shaders/blur.frag)
# [endregion]
qt_add_resources(example_gpu_lessons shared_media PREFIX "/media" BASE ../media/assets
    FILES ../media/assets/lesson.mkv)
