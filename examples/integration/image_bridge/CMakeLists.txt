qt_add_executable(example_image_bridge main.cpp ${PROJECT_SOURCE_DIR}/src/core/ScreenCapture.cpp)
target_include_directories(example_image_bridge PRIVATE ${PROJECT_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_image_bridge PRIVATE Qt6::Gui ${OpenCV_LIBS})
