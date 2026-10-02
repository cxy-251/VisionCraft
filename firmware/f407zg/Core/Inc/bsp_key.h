#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "main.h"

#define KEY0_PRES       1
#define KEY1_PRES       2
#define KEY2_PRES       3
#define WKUP_PRES       4

void Bsp_Key_Init(void);
uint8_t Bsp_Key_Scan(uint8_t mode);

#endif /* __BSP_KEY_H */
