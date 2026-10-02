#!/bin/bash
# 用 STM32CubeMX 的命令行重新生成固件工程（不打开图形界面）。
#   tools/cubemx_generate.sh [firmware/station/station.ioc]
#
# CubeMX 的交互命令行（-i）从标准输入读命令。它启动要 40 秒左右，之后更新器还会联网检查一阵，
# 期间命令会返回 "Updater is busy"，所以先等 100 秒再发命令。
set -e
IOC="$(realpath "${1:-$(dirname "$0")/../firmware/station/station.ioc}")"
CUBEMX="${CUBEMX:-$HOME/Applications/STM32CubeMX/STM32CubeMX}"
WAIT="${CUBEMX_WAIT:-100}"

echo "生成：$IOC（先等 CubeMX 启动 ${WAIT} 秒）"
cd "$(dirname "$CUBEMX")"
( sleep "$WAIT"
  echo "config load $IOC"
  echo "project generate"
  sleep 120
  echo "exit" ) | "$CUBEMX" -i 2>&1 | grep -E "OK$|KO$|OptionalMessage_ERROR|not ready" || true
echo "完成。用 git diff 查看 CubeMX 改了哪些文件。"
