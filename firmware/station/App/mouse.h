/* USB 鼠标：CubeMX 生成的 USB 主机库（HID 类）负责枚举和收报告，这里只把报告排进队列交给界面任务。
 * 见手册「指针事件：触摸和鼠标」。 */
#ifndef VC_MOUSE_H
#define VC_MOUSE_H
#include <stdint.h>

typedef struct {
    int8_t dx, dy;                         /* 这一次报告的移动量（向右、向下为正） */
    uint8_t buttons;                       /* bit0 左键，bit1 右键，bit2 中键 */
} mouse_report;

void mouse_init(void);                     /* 启动 USB 主机（会创建库自己的任务）；在任务里调用 */
int  mouse_take(mouse_report *r);          /* 取一个报告，没有返回 0，不等待 */
int  mouse_connected(void);                /* 鼠标已枚举完成、正在工作 */

extern volatile uint32_t g_mouse_reports;  /* 一共收到多少个报告 */
extern volatile uint32_t g_mouse_dropped;  /* 队列满丢掉的报告 */
#endif
