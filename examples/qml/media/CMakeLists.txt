qt_add_executable(example_media_lessons main.cpp)
qt_add_qml_module(example_media_lessons
    URI MediaLessons
    QML_FILES Main.qml
    SOURCES FrameProbe.h FrameProbe.cpp
        ../../../src/app/ImageView.h ../../../src/app/ImageView.cpp)
target_sources(example_media_lessons PRIVATE ../../../src/core/ScreenCapture.cpp)
target_include_directories(example_media_lessons PRIVATE ../../../src ../../../src/app)
target_link_libraries(example_media_lessons PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Concurrent opencv_core opencv_imgproc)
qt_add_resources(example_media_lessons media_samples PREFIX "/media" BASE assets
    FILES assets/tone.wav assets/lesson.mkv)
