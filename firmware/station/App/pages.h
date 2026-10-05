/* 工位屏的各个页面（gui.h 框架之上）。只由 StationTask 调用。 */
#ifndef VC_PAGES_H
#define VC_PAGES_H

#include "gui.h"
#include <stdint.h>

extern const gui_page g_page_home;
extern const gui_page g_page_station;
extern const gui_page g_page_sensors;
extern const gui_page g_page_device;

/* 不管当前显示哪一页，StationTask 都要周期调用：取走上位机发来的结果 / 缩略图、记录传感器历史 */
void pages_background(uint32_t now_ms);

#endif
