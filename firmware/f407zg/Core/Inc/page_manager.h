#ifndef __PAGE_MANAGER_H
#define __PAGE_MANAGER_H

#include "main.h"

#define PAGE_COUNT  7

typedef enum {
    TOUCH_EVENT_UP   = 0,
    TOUCH_EVENT_DOWN = 1,
    TOUCH_EVENT_MOVE = 2
} Touch_Event_t;

typedef struct {
    const char *title;
    void (*init)(void);
    void (*update)(void);
    void (*on_key)(uint8_t key);
    void (*on_touch)(uint16_t x, uint16_t y, uint8_t event);
} Page_t;

void Page_Manager_Init(void);
void Page_Manager_Update(void);
void Page_Manager_OnKey(uint8_t key);
void Page_Manager_OnTouch(uint16_t x, uint16_t y, uint8_t event);
void Page_Set(uint8_t idx);
void Page_Next(void);
void Page_Prev(void);

#endif /* __PAGE_MANAGER_H */
