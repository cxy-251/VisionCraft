#ifndef __BSP_REMOTE_H
#define __BSP_REMOTE_H

#include "main.h"

#define REMOTE_PIN      GPIO_PIN_11
#define REMOTE_PORT     GPIOG

void Bsp_Remote_Init(void);
uint8_t Bsp_Remote_Scan(uint8_t *p_addr, uint8_t *p_cmd);
const char* Bsp_Remote_GetKeyName(uint8_t cmd);

#endif /* __BSP_REMOTE_H */
