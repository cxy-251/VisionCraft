#ifndef __BSP_TERMINAL_H
#define __BSP_TERMINAL_H

#include "main.h"

#define TERM_MAX_LINES      18
#define TERM_LINE_LEN       54
#define TERM_CMD_MAX_LEN    48

void Bsp_Terminal_Init(void);
void Bsp_Terminal_DrawView(void);
void Bsp_Terminal_Update(void);
void Bsp_Terminal_OnTouch(uint16_t x, uint16_t y, uint8_t event);
void Bsp_Terminal_OnKey(uint8_t key);
void Bsp_Terminal_Print(const char *str);
void Bsp_Terminal_ExecuteCommand(const char *cmd_line);

#endif /* __BSP_TERMINAL_H */
