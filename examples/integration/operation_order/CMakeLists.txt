qt_add_executable(example_operation_order main.cpp ${PROJECT_SOURCE_DIR}/src/vision/ImageOperations.cpp)
target_include_directories(example_operation_order PRIVATE ${PROJECT_SOURCE_DIR}/src/vision ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_operation_order PRIVATE Qt6::Core ${OpenCV_LIBS})
