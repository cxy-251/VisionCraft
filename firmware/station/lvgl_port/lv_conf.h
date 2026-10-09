/*
 * LVGL 配置。只写和默认值不一样、或者值得说明的项；其余由 LVGL 的 include/lvgl/config/lv_conf_internal.h 给默认值。
 * 板子和电脑上的模拟器（examples/gui_sim）共用这一份，所以两边画出来的像素一样。
 * 见手册「移植 LVGL」。
 */
#ifndef LV_CONF_H
#define LV_CONF_H

/* ---- 颜色 ---- */
#define LV_COLOR_FORMAT_DEFAULT LV_COLOR_FORMAT_RGB565   /* 和 NT35510 一致，绘图缓冲可以原样写进显存（v9.6 起取代 LV_COLOR_DEPTH） */

/* ---- 内存 ---- */
/* LVGL 自己的堆（控件、样式、临时图层都从这里分配）。用 LVGL 内置的分配器，不和 FreeRTOS、newlib 抢 */
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN
#if defined(__arm__)
/* 内存池在片内 CCM。试过放到外部 SRAM（256 KB）：内存够了，但整屏一帧慢了 50–70%（LVGL 画图时反复读写控件和样式）。
 * 现在的做法是只保留正在显示的那一页（离开就删掉控件），见手册「只保留当前页：LVGL 页面的建与删」 */
#define LV_MEM_SIZE (44u * 1024u)
#else
/* 电脑上的模拟器是 64 位程序，指针是 8 字节，同样的控件要多占将近一倍；44 KB 会在建控件时用光，
 * 停在 LV_ASSERT_MALLOC 的死循环里（实测）。所以模拟器给大一些，板子上的用量以板上测量为准 */
#define LV_MEM_SIZE (128u * 1024u)
#endif

/* LVGL 源码里只有内存池用这个属性：放进片内 CCM（链接脚本的 .ccmbss，不占 Flash、不清零）。电脑上的模拟器什么都不加 */
#if defined(__arm__)
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY __attribute__((section(".ccmbss")))
#endif

/* ---- 运行 ---- */
#define LV_USE_OS LV_OS_NONE               /* 只有 StationTask 调用 LVGL，不需要 LVGL 自己加锁 */
#define LV_DEF_REFR_PERIOD 20              /* 最多每 20 ms 刷新一次，和界面循环的节拍一样 */
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1             /* 内存池用光时停在断言里，而不是悄悄画错 */

/* ---- 绘制 ---- */
#define LV_USE_DRAW_SW 1                   /* 纯 CPU 画（F407 没有 DMA2D 图形加速） */
#define LV_DRAW_SW_COMPLEX 1               /* 圆角、阴影、渐变、抗锯齿 */
#define LV_USE_FLOAT 0

/* ---- 字体 ---- */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
/* 中文不用 LVGL 自带的思源黑体（LV_FONT_SOURCE_HAN_SANS_SC_16_CJK）：它只收了一部分常用字，
 * 「演、鼠、标、操」这些都没有。改用 tools/make_cjk_font.sh 按界面实际用到的字生成的 vc_font_cjk_20 */
#define LV_FONT_DEFAULT &lv_font_montserrat_20

/* ---- 主题 ---- */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1            /* 深色，和上位机、原来的工位界面一致 */

/* ---- 不需要的模块 ---- */
#define LV_USE_OBSERVER 0
#define LV_USE_SNAPSHOT 0
#define LV_USE_SYSMON 0
#define LV_BUILD_EXAMPLES 0
#define LV_BUILD_DEMOS 0

#endif
