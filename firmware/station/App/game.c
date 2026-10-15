#include "game.h"
#include "random.h"

#include "lcd.h"
#include "main.h"                              /* HAL_GetTick */

volatile uint32_t g_game_seed;
static uint32_t s_state = 2463534242u;

void game_seed(uint32_t seed) { s_state = seed ? seed : 2463534242u; }

void game_new_seed(void)
{
    /* 正常玩：优先硬件 RNG，失败或桌面模拟时用按下的毫秒数做种子，每局不一样。测试时调试器写 g_game_seed，每局一样，结果能复现 */
    if (g_game_seed) { game_seed(g_game_seed); return; }
    uint32_t seed = HAL_GetTick() * 2654435761u + 1u;
    uint32_t hardware;
    if (random_word(&hardware)) seed = hardware;
    game_seed(seed);
}

uint32_t game_rand(void)
{
    uint32_t x = s_state;                      /* xorshift32：三次移位异或，周期 2^32 − 1 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return s_state = x;
}

void game_cell(int x, int y, int size, uint16_t color)
{
    lcd_fill((uint16_t)(x + 1), (uint16_t)(y + 1), (uint16_t)(size - 2), (uint16_t)(size - 2), color);
}
