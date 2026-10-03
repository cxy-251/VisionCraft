qt_add_executable(example_opencv_fit_hull main.cpp)
target_include_directories(example_opencv_fit_hull PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_opencv_fit_hull PRIVATE Qt6::Core ${OpenCV_LIBS})
