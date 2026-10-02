#include "main.h"
#include "lcd.h"
#include "bsp_backlight.h"
#include "bsp_key.h"
#include "bsp_touch.h"
#include "page_manager.h"
#include "bsp_spi_flash.h"
#include "bsp_at24c02.h"
#include "bsp_fs_manager.h"
#include "bsp_beep.h"
#include "bsp_lsens.h"
#include "bsp_cpu_temp.h"
#include "bsp_remote.h"
#include "bsp_sram.h"
#include "bsp_wm8978.h"
#include "bsp_usb_mouse.h"
#include "bsp_rtc.h"
#include "bsp_rng.h"
#include "bsp_dac.h"
#include "bsp_rs485.h"
#include "bsp_rs232.h"
#include "bsp_iwdg.h"
#include "bsp_lan8720.h"
#include "bsp_terminal.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);

static void Task_Key(void *argument) {
    (void)argument;
    while (1) {
        uint8_t key = Bsp_Key_Scan(0);
        if (key != 0) {
            Page_Manager_OnKey(key);
        }

        // 联动红外遥控器 (PG11)
        uint8_t ir_addr = 0, ir_cmd = 0;
        if (Bsp_Remote_Scan(&ir_addr, &ir_cmd)) {
            Bsp_Beep_Click();
            if (ir_cmd == 0xA8) { // NEXT 键切下页
                Page_Next();
            } else if (ir_cmd == 0xC2) { // PREV 键切上页
                Page_Prev();
            } else if (ir_cmd == 0x22) { // + 键调亮背光
                uint8_t b = Bsp_Backlight_Get();
                Bsp_Backlight_Set((b <= 90) ? b + 10 : 100);
            } else if (ir_cmd == 0x90) { // - 键调暗背光
                uint8_t b = Bsp_Backlight_Get();
                Bsp_Backlight_Set((b >= 15) ? b - 10 : 5);
            }
        }

        Bsp_IWDG_Feed();

        extern volatile uint8_t s_uart_cmd_pending;
        extern char s_uart_cmd_buffer[128];
        if (s_uart_cmd_pending) {
            Bsp_Terminal_ExecuteCommand(s_uart_cmd_buffer);
            s_uart_cmd_pending = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

static void Task_Touch(void *argument) {
    (void)argument;
    uint8_t last_pressed = 0;
    while (1) {
        uint8_t pressed = Bsp_Touch_Scan();
        if (pressed) {
            if (!last_pressed) {
                Page_Manager_OnTouch(g_touch.x, g_touch.y, TOUCH_EVENT_DOWN);
                last_pressed = 1;
            } else {
                Page_Manager_OnTouch(g_touch.x, g_touch.y, TOUCH_EVENT_MOVE);
            }
        } else if (last_pressed) {
            Page_Manager_OnTouch(g_touch.x, g_touch.y, TOUCH_EVENT_UP);
            last_pressed = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void Task_UI(void *argument) {
    (void)argument;
    while (1) {
        Page_Manager_Update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void Task_USB(void *argument) {
    (void)argument;
    uint8_t last_m_left = 0;
    uint8_t last_m_right = 0;
    while (1) {
        Bsp_UsbMouse_Process();
        Bsp_UsbMouse_RenderCursor();

        const Bsp_UsbMouse_State_t *m = Bsp_UsbMouse_GetState();
        if (m->is_connected) {
            // 鼠标左键映射为触屏点击事件
            if (m->btn_left && !last_m_left) {
                Page_Manager_OnTouch(m->x, m->y, TOUCH_EVENT_DOWN);
                last_m_left = 1;
            } else if (m->btn_left && last_m_left) {
                Page_Manager_OnTouch(m->x, m->y, TOUCH_EVENT_MOVE);
            } else if (!m->btn_left && last_m_left) {
                Page_Manager_OnTouch(m->x, m->y, TOUCH_EVENT_UP);
                last_m_left = 0;
            }

            // 鼠标右键切页
            if (m->btn_right && !last_m_right) {
                Page_Next();
                last_m_right = 1;
            } else if (!m->btn_right && last_m_right) {
                last_m_right = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();

    // 1. 初始化液晶与背光 PWM (TIM12_CH2 / PB15)
    LCD_Init();
    Bsp_Backlight_Init();
    Bsp_Backlight_Set(80);

    // 2. 初始化按键与电容触控芯片 (GT9147)
    Bsp_Key_Init();
    Bsp_Touch_Init();

    // 3. 初始化板载外部存储 (W25Q128 SPI Flash, AT24C02 EEPROM, 1MB FSMC SRAM)
    Bsp_SpiFlash_Init();
    Bsp_At24c02_Init();
    Bsp_Sram_Init();

    // 4. 初始化板载物理硬件外设 (蜂鸣器 PF8, 光敏 PF7, CPU温度 ADC1, 红外 PG11)
    Bsp_Beep_Init();
    Bsp_Lsens_Init();
    Bsp_CpuTemp_Init();
    Bsp_Remote_Init();

    // 5. 初始化音频 CODEC (WM8978 + I2S2) 与 USB Host 鼠标
    Bsp_WM8978_Init();
    Bsp_WM8978_PlayTone(1000, 300); // 开机向耳机播放 300ms 1kHz 测试音
    Bsp_UsbMouse_Init();

    // 6. 初始化片内高精外设 (RTC 实时时钟, 硬件 RNG, PA4 片内 DAC)
    Bsp_RTC_Init();
    Bsp_RNG_Init();
    Bsp_DAC_Init();

    // 7. 初始化板载通信接口 (RS485 SP3485, RS232 SP3232)
    Bsp_RS485_Init(9600);
    Bsp_RS232_Init(115200);

    // 8. 初始化 FatFs 文件系统子系统 (MicroSD + SPI Flash)
    Bsp_FsManager_Init();

    // 8. 初始化多页面状态机
    Page_Manager_Init();

    // 8. 创建 FreeRTOS 调度任务
    xTaskCreate(Task_Touch, "Task_Touch", 256, NULL, 4, NULL);
    xTaskCreate(Task_USB,   "Task_USB",   512, NULL, 3, NULL);
    xTaskCreate(Task_Key,   "Task_Key",   1024, NULL, 3, NULL);
    xTaskCreate(Task_UI,    "Task_UI",    512, NULL, 2, NULL);

    // 5. 启动调度器
    vTaskStartScheduler();

    while (1) {
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // 配置主内部调压器输出电压
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 初始化 HSE (8MHz 外部晶振) 并配置 PLL 生成 168MHz 主频
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    // 初始化 CPU、AHB 和 APB 总线时钟
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void) {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

volatile uint8_t s_uart_cmd_pending = 0;
char s_uart_cmd_buffer[128];
static char s_uart1_rx_line[128];
static uint8_t s_uart1_rx_idx = 0;

void Post_Uart_Command_From_ISR(const char *cmd) {
    if (!s_uart_cmd_pending && cmd && strlen(cmd) > 0) {
        strncpy(s_uart_cmd_buffer, cmd, sizeof(s_uart_cmd_buffer) - 1);
        s_uart_cmd_buffer[sizeof(s_uart_cmd_buffer) - 1] = '\0';
        s_uart_cmd_pending = 1;
    }
}

void USART1_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET) {
        __HAL_UART_CLEAR_OREFLAG(&huart1);
    }
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) {
        uint8_t ch = (uint8_t)(huart1.Instance->DR & 0xFF);
        if (ch == '\r' || ch == '\n') {
            if (s_uart1_rx_idx > 0) {
                s_uart1_rx_line[s_uart1_rx_idx] = '\0';
                Post_Uart_Command_From_ISR(s_uart1_rx_line);
                s_uart1_rx_idx = 0;
            }
        } else if (s_uart1_rx_idx < sizeof(s_uart1_rx_line) - 1) {
            s_uart1_rx_line[s_uart1_rx_idx++] = (char)ch;
        }
        __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE);
    }
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // GPIO 端口时钟使能
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // 配置初始电平 (高电平默认熄灭)
    HAL_GPIO_WritePin(GPIOF, LED0_PIN | LED1_PIN, GPIO_PIN_SET);

    // 配置 PF9 (LED0) 和 PF10 (LED1) 为推挽输出
    GPIO_InitStruct.Pin = LED0_PIN | LED1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
        // 出错时快速闪烁 LED0 告警
        HAL_GPIO_TogglePin(LED0_GPIO_PORT, LED0_PIN);
        for (volatile int i = 0; i < 500000; i++);
    }
}
