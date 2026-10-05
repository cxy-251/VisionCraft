#!/bin/bash
# 用 STM32CubeMX 的命令行重新生成固件工程（不打开图形界面）。
#   tools/cubemx_generate.sh [firmware/station/station.ioc]
#
# -q 让 CubeMX 从脚本文件读命令、执行完退出。它会自己等启动和更新器检查结束，不用猜要等多久。
# 本机一次大约 3 分钟。生成的代码只覆盖 USER CODE BEGIN/END 之外的部分。
set -e
TOOLS="$(dirname "$(realpath "$0")")"
IOC="$(realpath "${1:-$(dirname "$0")/../firmware/station/station.ioc}")"
CUBEMX="${CUBEMX:-$HOME/Applications/STM32CubeMX/STM32CubeMX}"

# [region script]
SCRIPT="$(mktemp --suffix=.txt)"
trap 'rm -f "$SCRIPT"' EXIT
cat > "$SCRIPT" <<CMDS
config load $IOC
project generate
exit
CMDS
# [endregion]

echo "生成：$IOC（大约 3 分钟）"
cd "$(dirname "$CUBEMX")"
# [region run]
"$CUBEMX" -q "$SCRIPT" 2>&1 | grep -E "^OK$|^KO|ERROR|not ready" || true
# [endregion]
# CubeMX 会把库文件恢复原样，生成后重新打上我们的修正
"$TOOLS/patch_usbh_hid.sh" "$(dirname "$IOC")"
echo "完成。用 git diff 查看 CubeMX 改了哪些文件。"
