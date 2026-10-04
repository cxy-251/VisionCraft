qt_add_executable(example_qt_udp main.cpp)
target_link_libraries(example_qt_udp PRIVATE Qt6::Core Qt6::Network)
