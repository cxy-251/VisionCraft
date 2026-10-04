#!/bin/bash
# 把工位固件复制到临时目录，加上 ipc_bench.c 编译出实验固件（不改动 firmware/station 本身）。
#   tools/fw_bench/build_variant.sh <输出目录>      生成 <输出目录>/firmware/station/build/station.elf
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; ROOT="$(cd "$HERE/../.." && pwd)"; OUT="$1"
rm -rf "$OUT/firmware"; mkdir -p "$OUT/firmware"
rsync -a --exclude build "$ROOT/firmware/station/" "$OUT/firmware/station/"
ln -sfn "$ROOT/protocol" "$OUT/protocol"
cp "$HERE/ipc_bench.c" "$OUT/firmware/station/App/"
cd "$OUT/firmware/station"
# app_init 末尾启动测量任务；CMake 里加上这个源文件
sed -i 's|^    g_beepQueue = osMessageQueueNew(4, sizeof(uint16_t), NULL);|&\n    extern void ipc_bench_start(void); ipc_bench_start();|' App/app.c
sed -i 's|^    App/eeprom.c|&\n    App/ipc_bench.c|' CMakeLists.txt
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build 2>&1 | grep -E "error|FLASH:|RAM:"
