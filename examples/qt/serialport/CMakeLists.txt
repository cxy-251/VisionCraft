qt_add_executable(example_qt_serialport main.cpp)
target_link_libraries(example_qt_serialport PRIVATE Qt6::Core Qt6::SerialPort)
