#!/bin/bash
# 工位固件的副本 + touchterm.c：原来的 StationTask 改名不用，换成触屏终端。不改动 firmware/station 本身。
#   tools/fw_touchterm/build_variant.sh <输出目录>      生成 <输出目录>/firmware/station/build/station.elf
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../.." && pwd)"; OUT="$1"
rm -rf "$OUT/firmware"; mkdir -p "$OUT/firmware"
rsync -a --exclude build "$ROOT/firmware/station/" "$OUT/firmware/station/"
ln -sfn "$ROOT/protocol" "$OUT/protocol"
cp "$HERE/touchterm.c" "$OUT/firmware/station/App/"
cd "$OUT/firmware/station"
sed -i 's|^void StationTask(void \*argument)|void StationTask_unused(void *argument)|' App/station.c
sed -i 's|^    App/eeprom.c|&\n    App/touchterm.c|' CMakeLists.txt
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build 2>&1 | grep -E "error|warning|FLASH:|RAM:" || true
