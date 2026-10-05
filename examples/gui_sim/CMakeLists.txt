# 工位屏界面的模拟器：把固件里的界面代码（gui.c、各页面、station_ui.c、字库）原样编译到电脑上，
# 屏幕换成内存缓冲（sim_lcd.c），板子的其余部分换成假数据（sim_app.c）。
set(FW ${PROJECT_SOURCE_DIR}/firmware/station/App)
qt_add_executable(example_gui_sim main.cpp sim_lcd.c sim_app.c
    ${FW}/gui.c ${FW}/page_home.c ${FW}/page_station.c ${FW}/page_sensors.c ${FW}/page_device.c
    ${FW}/station_ui.c ${FW}/lcd_font.c)
target_include_directories(example_gui_sim PRIVATE include ${FW} ${PROJECT_SOURCE_DIR}/protocol)
target_link_libraries(example_gui_sim PRIVATE Qt6::Gui)
