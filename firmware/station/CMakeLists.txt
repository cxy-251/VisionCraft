cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME station)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# USB 主机库 HID 类的 SET_PROTOCOL 取值是反的，CubeMX 每次生成都会恢复原样。没修正就停下来，
# 否则鼠标能用但 X、Y 全错，很难想到是库的问题。见手册「指针事件：触摸和鼠标」
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c _usbh_hid)
if(NOT _usbh_hid MATCHES "VisionCraft 修改")
    message(FATAL_ERROR "usbh_hid.c 还没修正：先运行 tools/patch_usbh_hid.sh")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# 编译时间戳：一个总是「过期」的目标，每次编译都重新生成 build_stamp.c
add_custom_target(build_stamp
    COMMAND ${CMAKE_COMMAND} -DOUT=${CMAKE_CURRENT_BINARY_DIR}/build_stamp.c -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/build_stamp.cmake
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/build_stamp.c
)
add_dependencies(${CMAKE_PROJECT_NAME} build_stamp)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_BINARY_DIR}/build_stamp.c
    App/app.c
    App/link.c
    App/station.c
    App/env.c
    App/vc_rtt.c
    App/lcd.c
    App/lcd_font.c
    App/station_ui.c
    App/eeprom.c
    App/gui.c
    App/page_home.c
    App/page_station.c
    App/page_sensors.c
    App/page_device.c
    App/touch.c
    App/mouse.c
    App/cursor.c
    App/page_lvgl.c
    lvgl_port/lv_port.c
    lvgl_port/vc_font_cjk_20.c
    # 与上位机共用的协议代码
    ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol/vc_protocol.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    App
    lvgl_port
    ${CMAKE_CURRENT_SOURCE_DIR}/../../protocol
)

# 生成 .bin 并打印各段大小
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# LVGL：firmware/third_party/lvgl 原样的源码，配置在 lvgl_port/lv_conf.h。第三方代码，不看它的警告。
# 没用到的模块由 lv_conf.h 关掉，编译出来是空的；链接时 --gc-sections 再丢掉用不到的函数
set(LVGL ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/lvgl)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${LVGL}/src/*.c)
add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl SYSTEM PUBLIC ${LVGL} ${CMAKE_CURRENT_SOURCE_DIR}/lvgl_port)
target_compile_definitions(lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_compile_options(lvgl PRIVATE -w)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl

    # Add user defined libraries
)
