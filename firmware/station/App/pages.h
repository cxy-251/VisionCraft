/* 工位屏的各个页面（gui.h 框架之上）。只由 StationTask 调用。 */
#ifndef VC_PAGES_H
#define VC_PAGES_H

#include "gui.h"
#include "vc_protocol.h"
#include <stdint.h>

extern const gui_page g_page_home;
extern const gui_page g_page_station;
extern const gui_page g_page_sensors;
extern const gui_page g_page_device;
extern const gui_page g_page_lvgl;
extern const gui_page g_page_classic;           /* 二级首页：下面是手写框架版的三个页面 */
extern const gui_page g_page_classic_station;
extern const gui_page g_page_classic_sensors;
extern const gui_page g_page_classic_device;

/* 开机时调用一次：初始化 LVGL，建好所有 LVGL 页面的控件（页面不显示时数据也照样更新到控件上） */
void pages_init(void);
/* 按键（KEY0…WKUP）按下 / 松开：工位页显示最近一次按键 */
void pages_key(uint8_t key, uint8_t down);

/* page_station.c 和 page_classic.c 之间：新数据到了 */
void station_result(const vc_result *r);
void station_image(uint16_t w, uint16_t h, const uint16_t *pixels);
void station_key(uint8_t key, uint8_t down);
void lvgl_page_build(void);
void sensors_build(void);
void device_build(void);
void sensors_record(uint32_t now_ms);
void classic_sensors_record(uint32_t now_ms);
void classic_result(const vc_result *r);
void classic_thumbnail(uint16_t w, uint16_t h, const uint16_t *pixels);

/* 不管当前显示哪一页，StationTask 都要周期调用：取走上位机发来的结果 / 缩略图、记录传感器历史 */
void pages_background(uint32_t now_ms);

#endif
