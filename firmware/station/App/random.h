#ifndef VC_RANDOM_H
#define VC_RANDOM_H
#include <stdint.h>
extern volatile uint32_t g_rng_good, g_rng_bad, g_rng_last;
/* StationTask 调用；HAL 等待最多约2 ms，失败不返回旧样本。 */
int random_word(uint32_t *value);
#endif
