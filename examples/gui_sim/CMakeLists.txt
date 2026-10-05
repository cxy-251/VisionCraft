# 工位屏界面的模拟器：把固件里的界面代码（gui.c、各页面、station_ui.c、字库、LVGL 和它的移植层）原样编译到电脑上，
# 屏幕换成内存缓冲（sim_lcd.c），板子的其余部分换成假数据（sim_app.c）。
set(FW ${PROJECT_SOURCE_DIR}/firmware/station/App)
set(PORT ${PROJECT_SOURCE_DIR}/firmware/station/lvgl_port)
set(LVGL ${PROJECT_SOURCE_DIR}/firmware/third_party/lvgl)

# LVGL：和固件同一份源码、同一个 lv_conf.h。第三方代码，不开我们自己的警告选项
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${LVGL}/src/*.c)
add_library(gui_sim_lvgl STATIC ${LVGL_SOURCES})
target_include_directories(gui_sim_lvgl SYSTEM PUBLIC ${LVGL} ${PORT})
target_compile_definitions(gui_sim_lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_compile_options(gui_sim_lvgl PRIVATE -w)

qt_add_executable(example_gui_sim main.cpp sim_lcd.c sim_app.c
    ${FW}/gui.c ${FW}/page_home.c ${FW}/page_station.c ${FW}/page_sensors.c ${FW}/page_device.c ${FW}/page_lvgl.c ${FW}/page_classic.c
    ${FW}/station_ui.c ${FW}/lcd_font.c ${PORT}/lv_port.c ${PORT}/vc_font_cjk_20.c ${PORT}/vc_font_big_72.c)
target_include_directories(example_gui_sim PRIVATE include ${FW} ${PROJECT_SOURCE_DIR}/protocol)
target_link_libraries(example_gui_sim PRIVATE Qt6::Gui gui_sim_lvgl)
