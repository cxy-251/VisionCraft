/* 小游戏共用：格子画法、随机数、按键编号。游戏用手写框架直接往屏上画格子，每一步只重画变了的格子。
 * 见手册「小游戏：格子、节拍与只重画变了的格子」。 */
#ifndef VC_GAME_H
#define VC_GAME_H
#include <stdint.h>

/* 板上四个按键的编号（和 station.c 里 s_keys 的顺序一致） */
enum { GAME_KEY0, GAME_KEY1, GAME_KEY2, GAME_WKUP };

uint32_t game_rand(void);                 /* 伪随机数（xorshift32） */
void game_seed(uint32_t seed);            /* 测试用：固定种子，每局都一样 */
extern volatile uint32_t g_game_seed;     /* 调试器写非 0：下一局用这个种子 */
void game_new_seed(void);                 /* 开新局时调用：g_game_seed 非 0 就用它，否则用当前时刻 */

/* 画一个格子：(x, y) 左上角、边长 size，留 2 像素缝 */
void game_cell(int x, int y, int size, uint16_t color);
/* 三个游戏的整局状态：数据都在这里（离开页面再回来局面还在），也导出给测试和模拟器读 */
#define MINES_N 9
typedef struct {
    uint8_t mine[MINES_N][MINES_N];        /* 1 = 有雷 */
    uint8_t adj[MINES_N][MINES_N];         /* 周围 8 格有几颗雷 */
    uint8_t cell[MINES_N][MINES_N];        /* HIDDEN / SHOWN / FLAGGED */
    uint8_t dirty[MINES_N][MINES_N];       /* 这一格要重画 */
    uint8_t placed;            /* 雷布好了没有：第一下点完才布，保证第一下不是雷 */
    uint8_t status;            /* PLAYING / WON / LOST */
    uint8_t boomR, boomC;      /* 踩到的那一颗 */
    uint16_t revealed, flags;
    uint32_t moves;
} mines_game;
extern mines_game g_mines;

#define SNAKE_COLS 20
#define SNAKE_ROWS 20
typedef struct {
    uint8_t x[SNAKE_COLS * SNAKE_ROWS], y[SNAKE_COLS * SNAKE_ROWS];      /* 身体：环形缓冲，head 是蛇头的下标，身体往前数 len 格 */
    uint16_t head, len;
    uint8_t occupied[SNAKE_ROWS][SNAKE_COLS];      /* 哪些格子被身体占着：撞自己、放食物都要查 */
    uint8_t foodX, foodY;
    uint8_t dir, nextDir;              /* 下一步的方向：按键改的是 nextDir，走的时候才生效，免得一步之内掉头 */
    uint8_t status;
    uint16_t score;
    uint32_t steps;
} snake_game;
extern snake_game g_snake;

#define TETRIS_COLS 10
#define TETRIS_ROWS 20
typedef struct {
    uint8_t board[TETRIS_ROWS][TETRIS_COLS];         /* 已经落定的格子：0 空，1–7 是哪种方块的颜色 */
    int8_t cells[4][2];                /* 正在落的这一块：4 个格子相对 (x, y) 的位置（已经按当前角度转好） */
    int8_t x, y;
    uint8_t type, next, status;
    uint16_t lines;
    uint32_t score, pieces;
} tetris_game;
extern tetris_game g_tetris;
extern volatile uint32_t g_tetris_cells_drawn;
#endif
