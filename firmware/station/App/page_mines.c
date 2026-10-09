/* 扫雷：9×9、10 颗雷。点一下翻开，按住半秒插旗（或者切到 FLAG 模式再点）。
 * 用手写框架直接画格子：每一步只重画变了的格子。见手册「小游戏：格子、节拍与只重画变了的格子」。 */
#include "pages.h"

#include "game.h"
#include "lcd.h"
#include "main.h"

#include <string.h>

#define N      MINES_N            /* 整局状态的结构在 game.h */
#define MINES  10
#define CELL   48
#define X0     24
#define Y0     (GUI_BAR_H + 64)
#define LONG_PRESS_MS 400u

#define C_BG      RGB565(15, 23, 42)
#define C_HIDDEN  RGB565(71, 85, 105)
#define C_SHOWN   RGB565(30, 41, 59)
#define C_BOOM    RGB565(185, 28, 28)
#define C_TEXT    RGB565(241, 245, 249)
#define C_MUTED   RGB565(148, 163, 184)

enum { HIDDEN, SHOWN, FLAGGED };
enum { PLAYING, WON, LOST };

// [region model]
/* 整局的状态都在这里（离开这一页再回来，局面还在）。导出给测试读 */
mines_game g_mines;
// [endregion]

static int s_statusDirty, s_flagMode;

static void mark_all(void) { memset(g_mines.dirty, 1, sizeof g_mines.dirty); s_statusDirty = 1; }

static void new_game(void)
{
    memset(&g_mines, 0, sizeof g_mines);
    game_new_seed();
    mark_all();
}

// [region rules]
/* 布雷：避开第一下点的格子和它周围一圈，第一下翻开的就是一片空地 */
static void place(int sr, int sc)
{
    int n = 0;
    while (n < MINES) {
        const int r = (int)(game_rand() % N), c = (int)(game_rand() % N);
        if (g_mines.mine[r][c] || (r >= sr - 1 && r <= sr + 1 && c >= sc - 1 && c <= sc + 1))
            continue;
        g_mines.mine[r][c] = 1;
        n++;
    }
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            for (int dr = -1; dr <= 1; ++dr)
                for (int dc = -1; dc <= 1; ++dc) {
                    const int rr = r + dr, cc = c + dc;
                    if ((dr || dc) && rr >= 0 && rr < N && cc >= 0 && cc < N && g_mines.mine[rr][cc])
                        g_mines.adj[r][c]++;
                }
    g_mines.placed = 1;
}

static void show(int r, int c)
{
    g_mines.cell[r][c] = SHOWN;
    g_mines.dirty[r][c] = 1;
}

/* 翻开一格。周围没有雷就把邻居也翻开——用一个显式的栈，不用递归（任务栈有限） */
static void reveal(int r0, int c0)
{
    if (g_mines.cell[r0][c0] != HIDDEN)
        return;
    if (!g_mines.placed)
        place(r0, c0);
    if (g_mines.mine[r0][c0]) {
        g_mines.status = LOST;
        g_mines.boomR = (uint8_t)r0;
        g_mines.boomC = (uint8_t)c0;
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                if (g_mines.mine[r][c]) show(r, c);
        s_statusDirty = 1;
        return;
    }
    uint8_t stack[N * N][2];
    int top = 0;
    stack[top][0] = (uint8_t)r0; stack[top][1] = (uint8_t)c0; top++;
    while (top) {
        top--;
        const int r = stack[top][0], c = stack[top][1];
        if (g_mines.cell[r][c] != HIDDEN)
            continue;
        show(r, c);
        g_mines.revealed++;
        if (g_mines.adj[r][c])
            continue;
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                const int rr = r + dr, cc = c + dc;
                if (rr >= 0 && rr < N && cc >= 0 && cc < N && g_mines.cell[rr][cc] == HIDDEN && top < N * N) {
                    stack[top][0] = (uint8_t)rr; stack[top][1] = (uint8_t)cc; top++;
                }
            }
    }
    if (g_mines.revealed == N * N - MINES) {   /* 不是雷的格子全翻开了：赢 */
        g_mines.status = WON;
        for (int r = 0; r < N; ++r)
            for (int c = 0; c < N; ++c)
                if (g_mines.mine[r][c] && g_mines.cell[r][c] != FLAGGED) {
                    g_mines.cell[r][c] = FLAGGED;
                    g_mines.dirty[r][c] = 1;
                }
        g_mines.flags = MINES;
    }
    s_statusDirty = 1;
}

static void toggle_flag(int r, int c)
{
    if (g_mines.cell[r][c] == SHOWN)
        return;
    g_mines.cell[r][c] = g_mines.cell[r][c] == FLAGGED ? HIDDEN : FLAGGED;
    g_mines.flags = (uint16_t)(g_mines.flags + (g_mines.cell[r][c] == FLAGGED ? 1 : -1));
    g_mines.dirty[r][c] = 1;
    s_statusDirty = 1;
}
// [endregion]

// [region draw]
static void draw_cell(int r, int c)
{
    static const uint16_t kNum[9] = {0, RGB565(96, 165, 250), RGB565(74, 222, 128), RGB565(248, 113, 113),
                                     RGB565(192, 132, 252), RGB565(251, 146, 60), RGB565(45, 212, 191),
                                     RGB565(241, 245, 249), RGB565(148, 163, 184)};
    const int x = X0 + c * CELL, y = Y0 + r * CELL;
    const uint8_t st = g_mines.cell[r][c];
    if (st != SHOWN) {
        game_cell(x, y, CELL, C_HIDDEN);
        if (st == FLAGGED)
            lcd_text((uint16_t)(x + 16), (uint16_t)(y + 8), "F", RGB565(248, 113, 113), C_HIDDEN, 2);
        return;
    }
    const int boom = g_mines.status == LOST && r == g_mines.boomR && c == g_mines.boomC;
    const uint16_t bg = boom ? C_BOOM : C_SHOWN;
    game_cell(x, y, CELL, bg);
    if (g_mines.mine[r][c])
        lcd_text((uint16_t)(x + 16), (uint16_t)(y + 8), "*", C_TEXT, bg, 2);
    else if (g_mines.adj[r][c]) {
        const char d[2] = {(char)('0' + g_mines.adj[r][c]), 0};
        lcd_text((uint16_t)(x + 16), (uint16_t)(y + 8), d, kNum[g_mines.adj[r][c]], bg, 2);
    }
}

static void draw_status(void)
{
    char line[32];
    const int left = MINES - g_mines.flags;
    static const char *const kState[] = {"PLAYING", "YOU WIN!", "BOOM"};
    int n = 0;
    line[n++] = 'M'; line[n++] = 'I'; line[n++] = 'N'; line[n++] = 'E'; line[n++] = 'S'; line[n++] = ' ';
    if (left < 0) line[n++] = '-';
    line[n++] = (char)('0' + (left < 0 ? -left : left) / 10 % 10);
    line[n++] = (char)('0' + (left < 0 ? -left : left) % 10);
    line[n++] = ' '; line[n++] = ' ';
    line[n] = 0;
    lcd_fill(0, GUI_BAR_H + 8, LCD_W, 40, C_BG);
    lcd_text(24, GUI_BAR_H + 14, line, C_MUTED, C_BG, 2);
    lcd_text(272, GUI_BAR_H + 14, kState[g_mines.status],
             g_mines.status == LOST ? RGB565(248, 113, 113) : g_mines.status == WON ? RGB565(74, 222, 128) : C_TEXT, C_BG, 2);
}
// [endregion]

/* 按钮：新开一局、切换「翻开 / 插旗」模式 */
static void on_new(gui_widget *w) { (void)w; new_game(); }
static void on_mode(gui_widget *w)
{
    s_flagMode = !s_flagMode;
    w->text = s_flagMode ? "FLAG" : "DIG";
    gui_invalidate(w);
}
static gui_widget s_buttons[] = {
    {24, Y0 + N * CELL + 24, 200, 72, "NEW", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_new, 0},
    {256, Y0 + N * CELL + 24, 200, 72, "DIG", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_mode, 0},
};

// [region pointer]
/* 棋盘不是控件：没按在按钮上的指针事件都交到这里，自己算是哪一格。
 * 在同一格上按下又抬起才算一下；按住超过 0.4 秒算长按（插旗） */
static int s_downR = -1, s_downC;
static uint32_t s_downT;

static int cell_at(int16_t x, int16_t y, int *r, int *c)
{
    if (x < X0 || y < Y0 || x >= X0 + N * CELL || y >= Y0 + N * CELL)
        return 0;
    *r = (y - Y0) / CELL;
    *c = (x - X0) / CELL;
    return 1;
}

static void mines_pointer(const gui_event *e)
{
    int r = 0, c = 0;
    if (e->type == GUI_DOWN) {
        s_downR = cell_at(e->x, e->y, &r, &c) ? r : -1;
        s_downC = c;
        s_downT = HAL_GetTick();
    } else if (e->type == GUI_UP) {
        if (s_downR >= 0 && cell_at(e->x, e->y, &r, &c) && r == s_downR && c == s_downC && g_mines.status == PLAYING) {
            const int longPress = HAL_GetTick() - s_downT >= LONG_PRESS_MS;
            if (longPress || s_flagMode) toggle_flag(r, c);
            else reveal(r, c);
            g_mines.moves++;
        }
        s_downR = -1;
    }
}
// [endregion]

static void mines_enter(void)
{
    if (g_mines.moves == 0 && !g_mines.placed) new_game();
    mark_all();                                /* 框架刚清了屏：整盘重画一次 */
}

static void mines_tick(uint32_t now)
{
    (void)now;
    for (int r = 0; r < N; ++r)
        for (int c = 0; c < N; ++c)
            if (g_mines.dirty[r][c]) {
                draw_cell(r, c);
                g_mines.dirty[r][c] = 0;
            }
    if (s_statusDirty) {
        draw_status();
        s_statusDirty = 0;
    }
}

static int mines_key(uint8_t key, uint8_t down)
{
    if (key == GAME_KEY0 && down) { new_game(); return 1; }   /* KEY0：新开一局 */
    return 0;
}

const gui_page g_page_mines = {"MINES", s_buttons, 2, mines_enter, mines_tick, mines_pointer, 0, mines_key};
