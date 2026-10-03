qt_add_executable(example_opencv_masks main.cpp)
target_include_directories(example_opencv_masks PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_masks PRIVATE Qt6::Core ${OpenCV_LIBS})
