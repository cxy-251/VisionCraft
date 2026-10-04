qt_add_executable(example_qt_network main.cpp)
target_link_libraries(example_qt_network PRIVATE Qt6::Core Qt6::Network)
