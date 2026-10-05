#include "mouse.h"

#include "cmsis_os2.h"
#include "usb_host.h"
#include "usbh_hid.h"

extern USBH_HandleTypeDef hUsbHostFS;
extern ApplicationTypeDef Appli_state;     /* usb_host.c：库的连接 / 断开回调里更新 */

volatile uint32_t g_mouse_reports;
volatile uint32_t g_mouse_dropped;
static osMessageQueueId_t s_queue;

void mouse_init(void)
{
    s_queue = osMessageQueueNew(32, sizeof(mouse_report), NULL);
    MX_USB_HOST_Init();
}

int mouse_connected(void) { return Appli_state == APPLICATION_READY && USBH_HID_GetDeviceType(&hUsbHostFS) == HID_MOUSE; }

int mouse_take(mouse_report *r) { return s_queue && osMessageQueueGet(s_queue, r, NULL, 0) == osOK; }

// [region callback]
/* 库每收到一个中断端点的报告就调用一次（在库的 USBH 任务里，不在中断里）。
 * 库里的同名函数是 __weak 的，这里的定义会替换它 */
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost)
{
    if (USBH_HID_GetDeviceType(phost) != HID_MOUSE)
        return;
    HID_MOUSE_Info_TypeDef *m;
    while ((m = USBH_HID_GetMouseInfo(phost)) != NULL) {   /* 每次从库的 FIFO 取一个报告 */
        const mouse_report r = {
            (int8_t)m->x, (int8_t)m->y,                    /* 库把有符号的移动量存成 uint8_t，要转回来 */
            (uint8_t)(m->buttons[0] | m->buttons[1] << 1 | m->buttons[2] << 2),
        };
        g_mouse_reports++;
        if (osMessageQueuePut(s_queue, &r, 0, 0) != osOK)
            g_mouse_dropped++;
    }
}
// [endregion]
