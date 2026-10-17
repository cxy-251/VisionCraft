qt_add_executable(example_latest_result main.cpp
    ${PROJECT_SOURCE_DIR}/src/core/ScreenCapture.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/ImageOperations.cpp)
target_include_directories(example_latest_result PRIVATE ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/vision ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_latest_result PRIVATE Qt6::Gui Qt6::Concurrent ${OpenCV_LIBS})
