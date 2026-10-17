qt_add_executable(example_method_choices main.cpp)
target_include_directories(example_method_choices PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_method_choices PRIVATE ${OpenCV_LIBS})
