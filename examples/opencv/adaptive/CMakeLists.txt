qt_add_executable(example_opencv_adaptive main.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp
    ${PROJECT_SOURCE_DIR}/src/demos/opencv/ThresholdDemo.cpp)
target_include_directories(example_opencv_adaptive PRIVATE
    ${PROJECT_SOURCE_DIR}/src/vision ${PROJECT_SOURCE_DIR}/src/demos/opencv ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_adaptive PRIVATE Qt6::Core Qt6::Gui Qt6::Qml ${OpenCV_LIBS})
