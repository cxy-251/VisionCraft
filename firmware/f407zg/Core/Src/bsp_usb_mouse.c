#include "bsp_usb_mouse.h"
#include "usbh_core.h"
#include "usbh_hid.h"
#include "lcd.h"

static USBH_HandleTypeDef s_hUSBHost;
static Bsp_UsbMouse_State_t s_mouse_state = {
    .x = 240,
    .y = 400,
    .dx = 0,
    .dy = 0,
    .btn_left = 0,
    .btn_right = 0,
    .btn_middle = 0,
    .is_connected = 0,
    .packet_count = 0
};

static int16_t s_rendered_x = -1;
static int16_t s_rendered_y = -1;

static const uint8_t s_cursor_shape[8][8] = {
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0},
    {1, 2, 1, 0, 0, 0, 0, 0},
    {1, 2, 2, 1, 0, 0, 0, 0},
    {1, 2, 2, 2, 1, 0, 0, 0},
    {1, 2, 2, 1, 0, 0, 0, 0},
    {1, 0, 1, 2, 1, 0, 0, 0},
    {0, 0, 0, 1, 1, 0, 0, 0}
};

static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id) {
    UNUSED(phost);
    switch (id) {
        case HOST_USER_CONNECTION:
            break;
        case HOST_USER_DISCONNECTION:
            s_mouse_state.is_connected = 0;
            break;
        case HOST_USER_CLASS_ACTIVE:
            s_mouse_state.is_connected = 1;
            break;
        default:
            break;
    }
}

extern uint8_t mouse_rx_report_buf[16];

void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) {
    if (USBH_HID_GetDeviceType(phost) == HID_MOUSE) {
        USBH_HID_GetMouseInfo(phost);

        // 无线鼠标接收器上报格式:
        // Byte 0: 按键状态 (bit0=左键, bit1=右键, bit2=中键)
        // Byte 2: X 轴相对位移 (int8_t)
        // Byte 4: Y 轴相对位移 (int8_t)
        s_mouse_state.btn_left   = mouse_rx_report_buf[0] & 0x01;
        s_mouse_state.btn_right  = (mouse_rx_report_buf[0] >> 1) & 0x01;
        s_mouse_state.btn_middle = (mouse_rx_report_buf[0] >> 2) & 0x01;

        int16_t dx = (int8_t)mouse_rx_report_buf[2];
        int16_t dy = (int8_t)mouse_rx_report_buf[4];

        s_mouse_state.dx = (int8_t)dx;
        s_mouse_state.dy = (int8_t)dy;

        s_mouse_state.x += dx;
        s_mouse_state.y += dy;

        // 限制在 LCD 坐标范围内 (480x800 分辨率)
        if (s_mouse_state.x < 0) s_mouse_state.x = 0;
        if (s_mouse_state.x > 470) s_mouse_state.x = 470;
        if (s_mouse_state.y < 0) s_mouse_state.y = 0;
        if (s_mouse_state.y > 790) s_mouse_state.y = 790;

        s_mouse_state.packet_count++;
        s_mouse_state.is_connected = 1;
    }
}

void Bsp_UsbMouse_Init(void) {
    USBH_Init(&s_hUSBHost, USBH_UserProcess, 0);
    USBH_RegisterClass(&s_hUSBHost, USBH_HID_CLASS);
    USBH_Start(&s_hUSBHost);
}

void Bsp_UsbMouse_Process(void) {
    USBH_Process(&s_hUSBHost);
}

const Bsp_UsbMouse_State_t* Bsp_UsbMouse_GetState(void) {
    return &s_mouse_state;
}

static uint16_t s_bg_buffer[8][8];
static uint8_t  s_has_saved_bg = 0;

void Bsp_UsbMouse_RenderCursor(void) {
    if (!s_mouse_state.is_connected && s_mouse_state.packet_count == 0) {
        return;
    }

    int16_t cur_x = s_mouse_state.x;
    int16_t cur_y = s_mouse_state.y;

    if (cur_x == s_rendered_x && cur_y == s_rendered_y) {
        return;
    }

    LCD_Lock();

    // 1. 恢复旧坐标处被光标覆盖的原 UI 像素 (仅恢复光标非透明点)
    if (s_rendered_x >= 0 && s_rendered_y >= 0 && s_has_saved_bg) {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (s_cursor_shape[r][c] != 0) {
                    LCD_Fast_DrawPoint(s_rendered_x + c, s_rendered_y + r, s_bg_buffer[r][c]);
                }
            }
        }
    }

    // 2. 保存新坐标处底层的原 UI 像素
    s_rendered_x = cur_x;
    s_rendered_y = cur_y;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            if (s_cursor_shape[r][c] != 0) {
                s_bg_buffer[r][c] = LCD_ReadPoint(cur_x + c, cur_y + r);
            }
        }
    }
    s_has_saved_bg = 1;

    // 3. 绘制新坐标处的 8x8 箭头光标 (左键按下变红)
    uint16_t fill_color = s_mouse_state.btn_left ? RED : WHITE;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            uint8_t pix = s_cursor_shape[r][c];
            if (pix == 1) {
                LCD_Fast_DrawPoint(cur_x + c, cur_y + r, BLACK);
            } else if (pix == 2) {
                LCD_Fast_DrawPoint(cur_x + c, cur_y + r, fill_color);
            }
        }
    }

    LCD_Unlock();
}

void Bsp_UsbMouse_ResetBg(void) {
    LCD_Lock();
    s_has_saved_bg = 0;
    s_rendered_x = -1;
    s_rendered_y = -1;
    LCD_Unlock();
}
