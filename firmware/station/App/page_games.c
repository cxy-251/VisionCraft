/* 「游戏」二级首页：和首页一样的图标，点进去是各个小游戏 */
#include "pages.h"

#include "lcd.h"

static void open_page(gui_widget *w) { gui_open((const gui_page *)w->user); }
static void draw_tile(const gui_widget *w);
static gui_widget s_tiles[] = {
    {16, GUI_BAR_H + 24, 144, 184, "MINES", RGB565(100, 116, 139), 0, 0, draw_tile, open_page, (void *)&g_page_mines},
    {168, GUI_BAR_H + 24, 144, 184, "SNAKE", RGB565(22, 163, 74), 0, 0, draw_tile, open_page, (void *)&g_page_snake},
    {320, GUI_BAR_H + 24, 144, 184, "TETRIS", RGB565(147, 51, 234), 0, 0, draw_tile, open_page, (void *)&g_page_tetris},
};
static const char *const kGlyph[] = {"MI", "SN", "TE"};
static void draw_tile(const gui_widget *w)
{
    gui_widget t = *w;
    t.user = (void *)kGlyph[w - s_tiles];
    gui_draw_tile(&t);
}
const gui_page g_page_games = {"GAMES", s_tiles, sizeof(s_tiles) / sizeof(s_tiles[0]), 0, 0};
