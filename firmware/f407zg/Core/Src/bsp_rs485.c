#include "bsp_rs485.h"
#include <string.h>

#define RS485_RX_BUF_SIZE 128

static UART_HandleTypeDef s_huart2;
static uint8_t s_rx_buf[RS485_RX_BUF_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;
static uint8_t s_rs485_ok = 0;

uint8_t Bsp_RS485_Init(uint32_t baudrate) {
    if (baudrate == 0) baudrate = 9600;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    // 1. PG8 配置为推挽输出 (RE/DE 收发控制)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    // 默认进入接收模式 (低电平)
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_RESET);

    // 2. PA2 (TX), PA3 (RX) 配置为 AF7 (USART2)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. USART2 外设初始化
    s_huart2.Instance = USART2;
    s_huart2.Init.BaudRate = baudrate;
    s_huart2.Init.WordLength = UART_WORDLENGTH_8B;
    s_huart2.Init.StopBits = UART_STOPBITS_1;
    s_huart2.Init.Parity = UART_PARITY_NONE;
    s_huart2.Init.Mode = UART_MODE_TX_RX;
    s_huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    s_huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&s_huart2) != HAL_OK) {
        s_rs485_ok = 0;
        return 1;
    }

    // 4. 开启 RXNE 中断
    __HAL_UART_ENABLE_IT(&s_huart2, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    s_rs485_ok = 1;
    return 0;
}

void Bsp_RS485_Send(const uint8_t *buf, uint16_t len) {
    if (!s_rs485_ok || !buf || len == 0) return;

    // 拉高 PG8 进入发送模式
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET);

    HAL_UART_Transmit(&s_huart2, (uint8_t *)buf, len, 1000);

    // 严格等待发送移位寄存器清空 (TC) 避免最后一个字节被截断
    while (__HAL_UART_GET_FLAG(&s_huart2, UART_FLAG_TC) == RESET);

    // 拉低 PG8 切回接收模式
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_RESET);
}

void Bsp_RS485_SendString(const char *str) {
    if (!str) return;
    Bsp_RS485_Send((const uint8_t *)str, (uint16_t)strlen(str));
}

uint16_t Bsp_RS485_Receive(uint8_t *buf, uint16_t max_len) {
    if (!buf || max_len == 0) return 0;
    uint16_t count = 0;
    while (s_rx_head != s_rx_tail && count < max_len) {
        buf[count++] = s_rx_buf[s_rx_tail];
        s_rx_tail = (s_rx_tail + 1) % RS485_RX_BUF_SIZE;
    }
    return count;
}

uint8_t Bsp_RS485_Available(void) {
    return (s_rx_head != s_rx_tail) ? 1 : 0;
}

void Bsp_RS485_Flush(void) {
    s_rx_tail = s_rx_head;
}

void USART2_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&s_huart2, UART_FLAG_RXNE) != RESET) {
        uint8_t byte = (uint8_t)(s_huart2.Instance->DR & 0xFF);
        uint16_t next_head = (s_rx_head + 1) % RS485_RX_BUF_SIZE;
        if (next_head != s_rx_tail) {
            s_rx_buf[s_rx_head] = byte;
            s_rx_head = next_head;
        }
        __HAL_UART_CLEAR_FLAG(&s_huart2, UART_FLAG_RXNE);
    }
}
