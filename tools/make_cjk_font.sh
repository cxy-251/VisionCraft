#!/bin/bash
# 生成 LVGL 用的中文字体：只收界面里真正用到的字，而不是几千个常用字。
#   tools/make_cjk_font.sh          → firmware/station/lvgl_port/vc_font_cjk_20.c
# 用到的字从 LVGL 页面源码的字符串常量里收集（注释里的字不算）。改了界面文字，重新运行一次。
# 需要：lv_font_conv（npm，装在 ~/Applications/lv_font_conv）、fonttools（~/Applications/fonttools，
# 用来从系统的 Noto Sans CJK 合集 .ttc 里取出简体中文那一个，lv_font_conv 不认 .ttc）。
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONV="$HOME/Applications/lv_font_conv/node_modules/.bin/lv_font_conv"
PY="$HOME/Applications/fonttools/bin/python"
TTC=/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc
SRC=("$ROOT/firmware/station/App/page_lvgl.c")
OUT="$ROOT/firmware/station/lvgl_port/vc_font_cjk_20.c"
TMP="$(mktemp -d)"; trap 'rm -rf "$TMP"' EXIT

# [region collect]
"$PY" - "$TTC" "$TMP/sc.otf" <<'PY'
import sys
from fontTools.ttLib import TTCollection
for f in TTCollection(sys.argv[1]).fonts:            # 合集里有日、韩、简、繁、港五种写法，取简体
    if f["name"].getDebugName(4) == "Noto Sans CJK SC":
        f.save(sys.argv[2]); break
else:
    sys.exit("没在 .ttc 里找到 Noto Sans CJK SC")
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
