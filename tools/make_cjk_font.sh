#!/bin/bash
# 生成 LVGL 用的中文字体：只收界面里真正用到的字，而不是几千个常用字。
#   tools/make_cjk_font.sh          → firmware/station/lvgl_port/vc_font_cjk_20.c、vc_font_big_72.c、vc_font_mono_16.c
# 用到的字从 LVGL 页面源码的字符串常量里收集（注释里的字不算）。改了界面文字，重新运行一次。
# 需要：lv_font_conv（npm，装在 ~/Applications/lv_font_conv）、fonttools（~/Applications/fonttools，
# 用来从系统的 Noto Sans CJK 合集 .ttc 里取出简体中文那一个，lv_font_conv 不认 .ttc）。
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONV="$HOME/Applications/lv_font_conv/node_modules/.bin/lv_font_conv"
PY="$HOME/Applications/fonttools/bin/python"
TTC=/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc
SRC=("$ROOT"/firmware/station/App/page_{lvgl,station,sensors,device,term}.c)
OUT="$ROOT/firmware/station/lvgl_port/vc_font_cjk_20.c"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

# [region collect]
"$PY" - "$TTC" "$TMP/sc.otf" "$TMP/mono.otf" <<'PY'
import sys
from fontTools.ttLib import TTCollection
want = {"Noto Sans CJK SC": sys.argv[2], "Noto Sans Mono CJK SC": sys.argv[3]}   # 简体；等宽版给终端页
for f in TTCollection(sys.argv[1]).fonts:            # 合集里有日、韩、简、繁、港五种写法，还各有等宽版
    name = f["name"].getDebugName(4)
    if name in want:
        f.save(want.pop(name))
if want:
    sys.exit("没在 .ttc 里找到：" + "、".join(want))
PY
SYMBOLS="$(python3 - "${SRC[@]}" <<'PY'
import re, sys
chars = set()
for path in sys.argv[1:]:
    code = re.sub(r"/\*.*?\*/|//[^\n]*", "", open(path, encoding="utf-8").read(), flags=re.S)   # 去掉注释
    for s in re.findall(r'"((?:[^"\\]|\\.)*)"', code):                                          # 字符串常量
        chars.update(c for c in s if ord(c) > 0x7E)
print("".join(sorted(chars)), end="")
PY
)"
# [endregion]
echo "收进字体的非 ASCII 字符（${#SYMBOLS} 个）：$SYMBOLS"
# [region convert]
"$CONV" --font "$TMP/sc.otf" -r 0x20-0x7E --symbols "$SYMBOLS" --size 20 --bpp 4 --format lvgl \
        --lv-font-name vc_font_cjk_20 --lv-include lvgl.h --no-compress -o "$OUT"
# [endregion]
echo "→ $OUT（$(stat -c %s "$OUT") 字节的 C 源码）"
# 工位页的结果大字：72 号只收 O、K、N、G 和横线，一个字一个字地收，比整套 ASCII 小得多
BIG="$ROOT/firmware/station/lvgl_port/vc_font_big_72.c"
"$CONV" --font "$TMP/sc.otf" --symbols "OKNG-" --size 72 --bpp 4 --format lvgl \
        --lv-font-name vc_font_big_72 --lv-include lvgl.h --no-compress -o "$BIG"
echo "→ $BIG（$(stat -c %s "$BIG") 字节的 C 源码）"
# 终端页的等宽字体：16 号，只有 ASCII。每个字一样宽，ps 之类的表格才对得齐
MONO="$ROOT/firmware/station/lvgl_port/vc_font_mono_16.c"
"$CONV" --font "$TMP/mono.otf" -r 0x20-0x7E --size 16 --bpp 4 --format lvgl \
        --lv-font-name vc_font_mono_16 --lv-include lvgl.h --no-compress -o "$MONO"
echo "→ $MONO（$(stat -c %s "$MONO") 字节的 C 源码）"
