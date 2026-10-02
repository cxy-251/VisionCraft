#ifndef __BSP_BACKLIGHT_H
#define __BSP_BACKLIGHT_H

#include "main.h"

void Bsp_Backlight_Init(void);
void Bsp_Backlight_Set(uint8_t percent);
uint8_t Bsp_Backlight_Get(void);

#endif /* __BSP_BACKLIGHT_H */
