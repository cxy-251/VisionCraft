qt_add_executable(example_teaching_case main.cpp)
target_include_directories(example_teaching_case PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(example_teaching_case PRIVATE ${OpenCV_LIBS})
