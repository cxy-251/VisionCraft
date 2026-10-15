#pragma once
#include <stdint.h>
typedef void *osMutexId_t;
typedef struct { uint32_t attr_bits; } osMutexAttr_t;
#define osMutexPrioInherit 2u
#define osOK 0
osMutexId_t osMutexNew(const osMutexAttr_t *);
int osMutexAcquire(osMutexId_t, uint32_t);
int osMutexRelease(osMutexId_t);
int osDelay(uint32_t);
