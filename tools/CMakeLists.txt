# 开发工具
qt_add_executable(vclink_probe vclink_probe.cpp
    ${PROJECT_SOURCE_DIR}/protocol/vc_protocol.c
    ${PROJECT_SOURCE_DIR}/src/device/DeviceLink.cpp
    ${PROJECT_SOURCE_DIR}/src/device/SimTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/SerialTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/RttTransport.cpp
    ${PROJECT_SOURCE_DIR}/src/device/Transport.h
)
target_include_directories(vclink_probe PRIVATE ${PROJECT_SOURCE_DIR}/protocol ${PROJECT_SOURCE_DIR}/src/device)
target_link_libraries(vclink_probe PRIVATE Qt6::Core Qt6::Network Qt6::SerialPort Qt6::Qml)
