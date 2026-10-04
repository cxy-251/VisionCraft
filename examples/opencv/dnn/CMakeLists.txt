find_package(OpenCV REQUIRED COMPONENTS core imgproc dnn)
qt_add_executable(example_opencv_dnn main.cpp ${PROJECT_SOURCE_DIR}/src/vision/PartGenerator.cpp)
target_include_directories(example_opencv_dnn PRIVATE ${PROJECT_SOURCE_DIR}/src/vision ${PROJECT_SOURCE_DIR}/protocol ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_dnn PRIVATE Qt6::Core opencv_core opencv_imgproc opencv_dnn)
