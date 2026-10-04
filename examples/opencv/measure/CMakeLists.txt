qt_add_executable(example_opencv_measure main.cpp ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp)
target_include_directories(example_opencv_measure PRIVATE ${PROJECT_SOURCE_DIR}/src/vision ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_measure PRIVATE Qt6::Core ${OpenCV_LIBS})
