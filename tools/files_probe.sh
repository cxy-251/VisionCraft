#!/bin/bash
# 从本次 ELF 解析地址并运行 FILES 只读实板回归。
# 默认复位后测试相机卡目录；--hotplug 等待手工插拔；--reinit 自动验证 CMD 断连恢复。
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
# 环境脚本允许原先没有 PATH 以外的变量，不启用 nounset 加载它。
set +u
source scripts/steamdeck-env.sh
set -u
ELF="$ROOT/firmware/station/build/Release/station.elf"
case "${1:-}" in
    "") MODE=normal ;;
    --hotplug) MODE=hotplug ;;
    --hotplug-trace) MODE=hotplug ;;
    --reinit) MODE=reinit ;;
    *) echo "usage: $0 [--hotplug|--hotplug-trace|--reinit]" >&2; exit 2 ;;
esac
SYMBOLS="$(mktemp)"
trap 'rm -f "$SYMBOLS"' EXIT
python3 - "$ELF" "$SYMBOLS" <<'PY'
import subprocess
import sys
from pathlib import Path
names = """g_gui_inject g_gui_title g_files_loads g_files_views g_files_thumbs g_files_binds
    g_lv_mem_used g_lv_mem_peak g_photo_last_rc g_sd_errors s_path s_open s_grid s_listRc
    s_ready s_mounted s_initMs uwTick g_reset_flags g_last_fault""".split()
found = {}
for line in subprocess.check_output(["arm-none-eabi-nm", sys.argv[1]], text=True).splitlines():
    fields = line.split()
    if len(fields) == 3 and fields[2] in names:
        found.setdefault(fields[2], []).append(fields[0])
for name in names:
    if len(found.get(name, [])) != 1:
        sys.exit(f"ELF symbol missing or ambiguous: {name}")
Path(sys.argv[2]).write_text("".join(f"set {name} 0x{found[name][0]}\n" for name in names))
PY
ARGS=(-f interface/stlink.cfg -f target/stm32f4x.cfg -c init)
if [[ "${1:-}" == --hotplug-trace ]]; then
    ARGS+=(-c 'set FILES_TRACE 1')
fi
if [[ "$MODE" == hotplug ]]; then
    ARGS+=(-c 'set FILES_HOTPLUG 1')
elif [[ "$MODE" == reinit ]]; then
    ARGS+=(-c 'set FILES_REINIT 1')
else
    ARGS+=(-c 'reset run' -c 'after 4000')
fi
openocd "${ARGS[@]}" -f "$SYMBOLS" -f tools/files_probe.tcl -c exit
