/* 板上命令行：解析一行命令、执行、把结果一行一行交给 out。和界面无关：
 * 终端页（page_term.c）和上位机链路（SHELL 命令，也在 page_term.c）调用同一个 shell_exec。见手册「终端页：命令解析与 LVGL 键盘」。 */
#ifndef VC_SHELL_H
#define VC_SHELL_H

typedef void (*shell_out)(const char *line, void *ctx);   /* 输出一行（不带换行） */

void shell_exec(const char *line, shell_out out, void *ctx);
const char *shell_complete(const char *prefix);           /* 以 prefix 开头的第一个命令名，没有返回 0 */

#endif
