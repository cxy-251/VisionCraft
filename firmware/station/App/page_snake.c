/* 贪吃蛇：20×20 格。方向：屏幕下方的按钮、板上按键、在棋盘上滑一下都行。越长越快。
 * 每走一步只重画两三格：蛇头新到的格子、蛇尾离开的格子、新出现的食物。见手册「小游戏」。 */
#include "pages.h"

#include "game.h"
#include "lcd.h"
#include "main.h"

#include <string.h>

#define COLS SNAKE_COLS          /* 整局状态的结构在 game.h */
#define ROWS SNAKE_ROWS
#define CELL 22
#define X0   20
#define Y0   (GUI_BAR_H + 64)
#define MAXLEN (COLS * ROWS)

#define C_BG    RGB565(15, 23, 42)
#define C_FIELD RGB565(30, 41, 59)
#define C_BODY  RGB565(74, 222, 128)
#define C_HEAD  RGB565(187, 247, 208)
#define C_FOOD  RGB565(248, 113, 113)
#define C_TEXT  RGB565(241, 245, 249)
#define C_MUTED RGB565(148, 163, 184)

enum { UP, RIGHT, DOWN, LEFT };
enum { READY, PLAYING, DEAD };

// [region model]
snake_game g_snake;
// [endregion]

static int s_fullRedraw, s_statusDirty;
static uint32_t s_lastStep;

static void cell(int x, int y, uint16_t color) { game_cell(X0 + x * CELL, Y0 + y * CELL, CELL, color); }

static void place_food(void)
{
    for (;;) {                         /* 随便挑一格，被身体占着就再挑（身体再长也总有空格，除非赢了） */
        const int x = (int)(game_rand() % COLS), y = (int)(game_rand() % ROWS);
        if (!g_snake.occupied[y][x]) {
            g_snake.foodX = (uint8_t)x;
            g_snake.foodY = (uint8_t)y;
            cell(x, y, C_FOOD);
            return;
        }
    }
}

static void new_game(void)
{
    memset(&g_snake, 0, sizeof g_snake);
    game_new_seed();
    g_snake.len = 3;
    g_snake.head = 2;
    for (int i = 0; i < 3; ++i) {      /* 中间一条横着的三格，朝右 */
        g_snake.x[i] = (uint8_t)(8 + i);
        g_snake.y[i] = 10;
        g_snake.occupied[10][8 + i] = 1;
    }
    g_snake.dir = g_snake.nextDir = RIGHT;
    g_snake.status = READY;            /* 按方向键或滑一下才开始 */
    place_food();
    s_fullRedraw = 1;
    s_statusDirty = 1;
}

// [region step]
static void step(void)
{
    static const int8_t dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
    g_snake.dir = g_snake.nextDir;
    const int hx = g_snake.x[g_snake.head] + dx[g_snake.dir], hy = g_snake.y[g_snake.head] + dy[g_snake.dir];
    const int eat = hx == g_snake.foodX && hy == g_snake.foodY;
    const uint16_t tail = (uint16_t)((g_snake.head + MAXLEN - (g_snake.len - 1)) % MAXLEN);
    /* 撞墙、撞自己就结束。蛇尾这一步会让开（没吃到东西时），所以撞到「当前的蛇尾」不算 */
    const int hitTail = hx == g_snake.x[tail] && hy == g_snake.y[tail] && !eat;
    if (hx < 0 || hy < 0 || hx >= COLS || hy >= ROWS || (g_snake.occupied[hy][hx] && !hitTail)) {
        g_snake.status = DEAD;
        s_statusDirty = 1;
        return;
    }
    if (!eat) {                        /* 没吃到：蛇尾离开它的格子，重画成空地 */
        g_snake.occupied[g_snake.y[tail]][g_snake.x[tail]] = 0;
        cell(g_snake.x[tail], g_snake.y[tail], C_FIELD);
    }
    cell(g_snake.x[g_snake.head], g_snake.y[g_snake.head], C_BODY);   /* 原来的蛇头变成身体 */
    g_snake.head = (uint16_t)((g_snake.head + 1) % MAXLEN);
    g_snake.x[g_snake.head] = (uint8_t)hx;
    g_snake.y[g_snake.head] = (uint8_t)hy;
    g_snake.occupied[hy][hx] = 1;
    cell(hx, hy, C_HEAD);
    g_snake.steps++;
    if (eat) {
        g_snake.len++;
        g_snake.score++;
        s_statusDirty = 1;
        place_food();
    }
}
// [endregion]

static void draw_all(void)
{
    lcd_fill(X0, Y0, COLS * CELL, ROWS * CELL, C_BG);
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x)
            cell(x, y, g_snake.occupied[y][x] ? C_BODY : C_FIELD);
    cell(g_snake.x[g_snake.head], g_snake.y[g_snake.head], C_HEAD);
    cell(g_snake.foodX, g_snake.foodY, C_FOOD);
}

static void draw_status(void)
{
    char line[24] = "SCORE ";
    int n = 6;
    const unsigned s = g_snake.score;
    if (s >= 100) line[n++] = (char)('0' + s / 100 % 10);
    if (s >= 10) line[n++] = (char)('0' + s / 10 % 10);
    line[n++] = (char)('0' + s % 10);
    while (n < 10) line[n++] = ' ';
    line[n] = 0;
    static const char *const kState[] = {"SWIPE TO GO", "  PLAYING  ", " GAME OVER "};
    lcd_text(20, GUI_BAR_H + 14, line, C_MUTED, C_BG, 2);
    lcd_text(276, GUI_BAR_H + 14, kState[g_snake.status], g_snake.status == DEAD ? C_FOOD : C_TEXT, C_BG, 2);
}

/* 改方向：不能直接掉头（会立刻撞上自己的脖子） */
static void turn(int d)
{
    if (g_snake.status == DEAD) return;
    if ((d + 2) % 4 == g_snake.dir && g_snake.len > 1) return;
    g_snake.nextDir = (uint8_t)d;
    if (g_snake.status == READY) { g_snake.status = PLAYING; s_statusDirty = 1; s_lastStep = HAL_GetTick(); }
}

static void on_dir(gui_widget *w) { turn((int)(intptr_t)w->user); }
static void on_new(gui_widget *w) { (void)w; new_game(); }
#define BY (Y0 + ROWS * CELL + 16)
static gui_widget s_buttons[] = {
    {20, BY, 100, 76, "<", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_dir, (void *)LEFT},
    {130, BY, 100, 76, "^", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_dir, (void *)UP},
    {240, BY, 100, 76, "v", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_dir, (void *)DOWN},
    {350, BY, 110, 76, ">", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_dir, (void *)RIGHT},
    {20, BY + 88, 440, 56, "NEW GAME", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_new, 0},
};

// [region swipe]
/* 在棋盘上滑一下：按下和抬起的位置之差，哪个方向的分量大就往哪走（至少滑 30 像素） */
static int16_t s_downX, s_downY;
static void snake_pointer(const gui_event *e)
{
    if (e->type == GUI_DOWN) { s_downX = e->x; s_downY = e->y; return; }
    if (e->type != GUI_UP) return;
    const int ddx = e->x - s_downX, ddy = e->y - s_downY;
    const int ax = ddx < 0 ? -ddx : ddx, ay = ddy < 0 ? -ddy : ddy;
    if (ax < 30 && ay < 30) return;
    if (ax > ay) turn(ddx > 0 ? RIGHT : LEFT);
    else turn(ddy > 0 ? DOWN : UP);
}
// [endregion]

static int snake_key(uint8_t key, uint8_t down)
{
    if (!down) return 1;
    switch (key) {                     /* 板上四个按键当方向键 */
    case GAME_WKUP: turn(UP); break;
    case GAME_KEY1: turn(DOWN); break;
    case GAME_KEY2: turn(LEFT); break;
    case GAME_KEY0: turn(RIGHT); break;
    }
    return 1;
}

static void snake_enter(void)
{
    if (g_snake.len == 0) new_game();
    if (g_snake.status == PLAYING) g_snake.status = READY;   /* 离开再回来：先停着，等方向键 */
    s_fullRedraw = 1;
    s_statusDirty = 1;
}

static void snake_tick(uint32_t now)
{
    if (s_fullRedraw) { draw_all(); s_fullRedraw = 0; }
    /* 节拍：开始 160 ms 走一步，每吃一个快 4 ms，最快 70 ms */
    const uint32_t period = g_snake.score * 4u >= 90u ? 70u : 160u - g_snake.score * 4u;
    /* 有符号差：turn() 里记的 s_lastStep 可能比这一轮的 now 晚（看门狗、调暗都踩过这个坑） */
    if (g_snake.status == PLAYING && (int32_t)(now - s_lastStep) >= (int32_t)period) {
        s_lastStep = now;
        step();
    }
    if (s_statusDirty) { draw_status(); s_statusDirty = 0; }
}

const gui_page g_page_snake = {"SNAKE", s_buttons, 5, snake_enter, snake_tick, snake_pointer, 0, snake_key};
