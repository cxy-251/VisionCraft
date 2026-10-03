qt_add_executable(example_opencv_pipeline_timing main.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/Inspector.cpp
    ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp)
target_include_directories(example_opencv_pipeline_timing PRIVATE
    ${PROJECT_SOURCE_DIR}/src/vision ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_pipeline_timing PRIVATE Qt6::Core ${OpenCV_LIBS})
