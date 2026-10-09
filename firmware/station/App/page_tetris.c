/* 俄罗斯方块：10×20。按钮或板上按键：左、右、旋转、下。消行越多越快。
 * 画法：记住屏上每一格现在是什么颜色（shown），每轮算出应该是什么，只重画不一样的格子。见手册「小游戏」。 */
#include "pages.h"

#include "game.h"
#include "lcd.h"
#include "main.h"

#include <string.h>

#define COLS TETRIS_COLS         /* 整局状态的结构在 game.h */
#define ROWS TETRIS_ROWS
#define CELL 26
#define X0   20
#define Y0   (GUI_BAR_H + 64)
#define PX   300                       /* 右边的信息栏 */

#define C_BG    RGB565(15, 23, 42)
#define C_FIELD RGB565(30, 41, 59)
#define C_TEXT  RGB565(241, 245, 249)
#define C_MUTED RGB565(148, 163, 184)

static const uint16_t kColor[8] = {
    C_FIELD, RGB565(34, 211, 238), RGB565(250, 204, 21), RGB565(192, 132, 252), RGB565(74, 222, 128),
    RGB565(248, 113, 113), RGB565(96, 165, 250), RGB565(251, 146, 60),
};

// [region pieces]
/* 七种方块：每种 4 个格子在一个 n×n 小方框里的位置。转一下就是绕方框中心转 90°：(x, y) → (n−1−y, x) */
static const struct { uint8_t n; int8_t c[4][2]; } kPiece[7] = {
    {4, {{0, 1}, {1, 1}, {2, 1}, {3, 1}}},   /* I */
    {2, {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},   /* O */
    {3, {{1, 0}, {0, 1}, {1, 1}, {2, 1}}},   /* T */
    {3, {{1, 0}, {2, 0}, {0, 1}, {1, 1}}},   /* S */
    {3, {{0, 0}, {1, 0}, {1, 1}, {2, 1}}},   /* Z */
    {3, {{0, 0}, {0, 1}, {1, 1}, {2, 1}}},   /* J */
    {3, {{2, 0}, {0, 1}, {1, 1}, {2, 1}}},   /* L */
};
// [endregion]

enum { PLAYING, OVER };

tetris_game g_tetris;
volatile uint32_t g_tetris_cells_drawn;   /* 一共重画过多少格（给测试读） */

static uint8_t s_shown[ROWS][COLS];    /* 屏上每一格现在画的是什么：和应该画的比，不一样才重画 */
static uint8_t s_shownNext = 0xFF;
static uint32_t s_lastFall, s_shownScore = 0xFFFFFFFF;
static uint8_t s_shownStatus = 0xFF;
static int s_full;

static int fits(const int8_t c[4][2], int x, int y)
{
    for (int i = 0; i < 4; ++i) {
        const int cx = x + c[i][0], cy = y + c[i][1];
        if (cx < 0 || cx >= COLS || cy >= ROWS || (cy >= 0 && g_tetris.board[cy][cx]))
            return 0;
    }
    return 1;
}

static void spawn(void)
{
    g_tetris.type = g_tetris.next;
    g_tetris.next = (uint8_t)(game_rand() % 7);
    memcpy(g_tetris.cells, kPiece[g_tetris.type].c, sizeof g_tetris.cells);
    g_tetris.x = 3;
    g_tetris.y = 0;
    g_tetris.pieces++;
    if (!fits(g_tetris.cells, g_tetris.x, g_tetris.y))
        g_tetris.status = OVER;        /* 新的一块一出来就放不下：结束 */
}

static void new_game(void)
{
    memset(&g_tetris, 0, sizeof g_tetris);
    game_new_seed();
    g_tetris.next = (uint8_t)(game_rand() % 7);
    spawn();
    s_lastFall = HAL_GetTick();
    s_full = 1;
}

// [region rules]
static int move(int dx, int dy)
{
    if (g_tetris.status != PLAYING || !fits(g_tetris.cells, g_tetris.x + dx, g_tetris.y + dy))
        return 0;
    g_tetris.x = (int8_t)(g_tetris.x + dx);
    g_tetris.y = (int8_t)(g_tetris.y + dy);
    return 1;
}

static void rotate(void)
{
    if (g_tetris.status != PLAYING) return;
    const int n = kPiece[g_tetris.type].n;
    int8_t r[4][2];
    for (int i = 0; i < 4; ++i) {
        r[i][0] = (int8_t)(n - 1 - g_tetris.cells[i][1]);
        r[i][1] = g_tetris.cells[i][0];
    }
    static const int8_t kick[] = {0, -1, 1, -2, 2};   /* 贴着墙转不开：往旁边挪一两格再试 */
    for (unsigned k = 0; k < sizeof kick; ++k)
        if (fits(r, g_tetris.x + kick[k], g_tetris.y)) {
            memcpy(g_tetris.cells, r, sizeof r);
            g_tetris.x = (int8_t)(g_tetris.x + kick[k]);
            return;
        }
}

/* 落不下去了：并进棋盘、消掉满的行、出下一块 */
static void lock(void)
{
    for (int i = 0; i < 4; ++i) {
        const int cy = g_tetris.y + g_tetris.cells[i][1];
        if (cy >= 0) g_tetris.board[cy][g_tetris.x + g_tetris.cells[i][0]] = (uint8_t)(g_tetris.type + 1);
    }
    int cleared = 0;
    for (int y = ROWS - 1; y >= 0; --y) {
        int full = 1;
        for (int x = 0; x < COLS; ++x) full &= g_tetris.board[y][x] != 0;
        if (!full) continue;
        memmove(&g_tetris.board[1], &g_tetris.board[0], (size_t)y * COLS);   /* 上面的行整体往下挪一行 */
        memset(&g_tetris.board[0], 0, COLS);
        cleared++;
        y++;                           /* 挪下来的这一行也要再检查 */
    }
    static const uint16_t kScore[5] = {0, 100, 300, 500, 800};
    g_tetris.score += (uint32_t)kScore[cleared] * (g_tetris.lines / 10u + 1u);
    g_tetris.lines = (uint16_t)(g_tetris.lines + cleared);
    spawn();
}

static void drop_one(void)
{
    if (g_tetris.status == PLAYING && !move(0, 1))
        lock();
}
// [endregion]

// [region draw]
static void draw_number(int x, int y, const char *label, uint32_t v)
{
    char d[12];
    int n = 0;
    do { d[n++] = (char)('0' + v % 10); v /= 10; } while (v && n < 10);
    char s[12];
    for (int i = 0; i < n; ++i) s[i] = d[n - 1 - i];
    while (n < 7) s[n++] = ' ';
    s[n] = 0;
    lcd_text((uint16_t)x, (uint16_t)y, label, C_MUTED, C_BG, 2);
    lcd_text((uint16_t)x, (uint16_t)(y + 32), s, C_TEXT, C_BG, 2);
}

static void draw(void)
{
    if (s_full) {
        memset(s_shown, 0xFF, sizeof s_shown);   /* 「屏上什么都不知道」：下面每格都会重画 */
        s_shownNext = 0xFF;
        s_shownScore = 0xFFFFFFFF;
        s_shownStatus = 0xFF;
        lcd_text(PX, Y0, "NEXT", C_MUTED, C_BG, 2);
        s_full = 0;
    }
    uint8_t want[ROWS][COLS];
    memcpy(want, g_tetris.board, sizeof want);
    for (int i = 0; i < 4; ++i) {      /* 正在落的这一块叠上去 */
        const int cx = g_tetris.x + g_tetris.cells[i][0], cy = g_tetris.y + g_tetris.cells[i][1];
        if (cy >= 0) want[cy][cx] = (uint8_t)(g_tetris.type + 1);
    }
    for (int y = 0; y < ROWS; ++y)
        for (int x = 0; x < COLS; ++x)
            if (want[y][x] != s_shown[y][x]) {   /* 只画和屏上不一样的格子 */
                game_cell(X0 + x * CELL, Y0 + y * CELL, CELL, kColor[want[y][x]]);
                s_shown[y][x] = want[y][x];
                g_tetris_cells_drawn++;
            }
    if (g_tetris.next != s_shownNext) {
        lcd_fill(PX, Y0 + 36, 4 * CELL, 2 * CELL, C_BG);
        for (int i = 0; i < 4; ++i)
            game_cell(PX + kPiece[g_tetris.next].c[i][0] * CELL, Y0 + 36 + kPiece[g_tetris.next].c[i][1] * CELL, CELL,
                      kColor[g_tetris.next + 1]);
        s_shownNext = g_tetris.next;
    }
    if (g_tetris.score != s_shownScore) {
        draw_number(PX, Y0 + 120, "SCORE", g_tetris.score);
        draw_number(PX, Y0 + 200, "LINES", g_tetris.lines);
        draw_number(PX, Y0 + 280, "LEVEL", g_tetris.lines / 10u + 1u);
        s_shownScore = g_tetris.score;
    }
    if (g_tetris.status != s_shownStatus) {    /* 文字也一样：变了才画 */
        lcd_text(PX, Y0 + 370, g_tetris.status == OVER ? "GAME OVER" : "         ", RGB565(248, 113, 113), C_BG, 2);
        s_shownStatus = g_tetris.status;
    }
}
// [endregion]

static void on_left(gui_widget *w) { (void)w; move(-1, 0); }
static void on_right(gui_widget *w) { (void)w; move(1, 0); }
static void on_rot(gui_widget *w) { (void)w; rotate(); }
static void on_down(gui_widget *w) { (void)w; drop_one(); }
static void on_new(gui_widget *w) { (void)w; new_game(); }
#define BY (Y0 + ROWS * CELL + 16)
static gui_widget s_buttons[] = {
    {20, BY, 100, 76, "<", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_left, 0},
    {130, BY, 100, 76, "ROT", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_rot, 0},
    {240, BY, 100, 76, "v", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_down, 0},
    {350, BY, 110, 76, ">", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_right, 0},
    {PX, Y0 + 420, 160, 72, "NEW", RGB565(30, 41, 59), 0, 0, gui_draw_button, on_new, 0},
};

static int tetris_key(uint8_t key, uint8_t down)
{
    if (!down) return 1;
    switch (key) {
    case GAME_KEY2: move(-1, 0); break;
    case GAME_KEY0: move(1, 0); break;
    case GAME_WKUP: rotate(); break;
    case GAME_KEY1: drop_one(); break;
    }
    return 1;
}

static void tetris_enter(void)
{
    if (g_tetris.pieces == 0) new_game();
    s_full = 1;
    s_lastFall = HAL_GetTick();
}

static void tetris_tick(uint32_t now)
{
    /* 自动下落：第 1 级 600 ms 一格，每升一级快 50 ms，最快 100 ms */
    const uint32_t level = g_tetris.lines / 10u;
    const uint32_t period = level >= 10u ? 100u : 600u - level * 50u;
    if (g_tetris.status == PLAYING && (int32_t)(now - s_lastFall) >= (int32_t)period) {
        s_lastFall = now;
        drop_one();
    }
    draw();
}

const gui_page g_page_tetris = {"TETRIS", s_buttons, 5, tetris_enter, tetris_tick, 0, 0, tetris_key};
