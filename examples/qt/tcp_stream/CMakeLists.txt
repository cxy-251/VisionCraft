qt_add_executable(example_qt_tcp_stream main.cpp ${PROJECT_SOURCE_DIR}/protocol/vc_protocol.c)
target_include_directories(example_qt_tcp_stream PRIVATE ${PROJECT_SOURCE_DIR}/protocol)
target_link_libraries(example_qt_tcp_stream PRIVATE Qt6::Core Qt6::Network)
