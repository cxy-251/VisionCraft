#include "bsp_rs232.h"
#include "bsp_terminal.h"
#include <string.h>

#define RS232_RX_BUF_SIZE 128

static UART_HandleTypeDef s_huart3;
static uint8_t s_rx_buf[RS232_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static uint8_t s_rs232_ok = 0;

uint8_t Bsp_RS232_Init(uint32_t baudrate) {
    if (baudrate == 0) baudrate = 115200;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    // PB10 (TX), PB11 (RX) 配置为 AF7 (USART3)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // USART3 外设初始化
    s_huart3.Instance = USART3;
    s_huart3.Init.BaudRate = baudrate;
    s_huart3.Init.WordLength = UART_WORDLENGTH_8B;
    s_huart3.Init.StopBits = UART_STOPBITS_1;
    s_huart3.Init.Parity = UART_PARITY_NONE;
    s_huart3.Init.Mode = UART_MODE_TX_RX;
    s_huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_huart3.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&s_huart3) != HAL_OK) {
        s_rs232_ok = 0;
        return 1;
    }

    // 开启 RXNE 中断
    __HAL_UART_ENABLE_IT(&s_huart3, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);

    s_rs232_ok = 1;
    return 0;
}

void Bsp_RS232_Send(const uint8_t *buf, uint16_t len) {
    if (!s_rs232_ok || !buf || len == 0) return;
    HAL_UART_Transmit(&s_huart3, (uint8_t *)buf, len, 1000);
}

void Bsp_RS232_SendString(const char *str) {
    if (!str) return;
    Bsp_RS232_Send((const uint8_t *)str, (uint16_t)strlen(str));
}

uint16_t Bsp_RS232_Receive(uint8_t *buf, uint16_t max_len) {
    if (!buf || max_len == 0) return 0;
    uint16_t count = 0;
    while (s_rx_head != s_rx_tail && count < max_len) {
        buf[count++] = s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % RS232_RX_BUF_SIZE;
    }
    return count;
}

uint8_t Bsp_RS232_Available(void) {
    return (s_rx_head != s_rx_tail) ? 1 : 0;
}

void Bsp_RS232_Flush(void) {
    s_rx_tail = s_rx_head;
}

static char s_rs232_cmd[128];
static uint8_t s_rs232_cmd_idx = 0;

void USART3_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&s_huart3, UART_FLAG_ORE) != RESET) {
        __HAL_UART_CLEAR_OREFLAG(&s_huart3);
    }
    if (__HAL_UART_GET_FLAG(&s_huart3, UART_FLAG_RXNE) != RESET) {
        uint8_t byte = (uint8_t)(s_huart3.Instance->DR & 0xFF);
        uint16_t next_head = (s_rx_head + 1) % RS232_RX_BUF_SIZE;
        if (next_head != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next_head;
        }

        if (byte == '\r' || byte == '\n') {
            if (s_rs232_cmd_idx > 0) {
                s_rs232_cmd[s_rs232_cmd_idx] = '\0';
                Post_Uart_Command_From_ISR(s_rs232_cmd);
                s_rs232_cmd_idx = 0;
            }
        } else if (s_rs232_cmd_idx < sizeof(s_rs232_cmd) - 1) {
            s_rs232_cmd[s_rs232_cmd_idx++] = (char)byte;
        }

        __HAL_UART_CLEAR_FLAG(&s_huart3, UART_FLAG_RXNE);
    }
}
