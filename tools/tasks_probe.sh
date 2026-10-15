#!/bin/bash
# 编译/烧录后运行；本脚本复位并仅验证设备页和任务信息，不访问存储页面。
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
set +u
source scripts/steamdeck-env.sh
set -u
SYMBOLS="$(mktemp)"
trap 'rm -f "$SYMBOLS"' EXIT
python3 - "$ROOT/firmware/station/build/Release/station.elf" "$SYMBOLS" <<'PY'
import subprocess
import sys
from pathlib import Path
names = """g_gui_inject g_gui_title g_device_tasks g_device_tasks_rc g_device_task_updates
 g_device_task_peak_cycles g_lv_mem_used g_lv_mem_peak g_term_inject g_term_inject_seq g_term_log
 g_reset_flags uwTick""".split()
found = {}
for line in subprocess.check_output(["arm-none-eabi-nm", sys.argv[1]], text=True).splitlines():
    fields = line.split()
    if len(fields) == 3 and fields[2] in names:
        found.setdefault(fields[2], []).append(fields[0])
for name in names:
    if len(found.get(name, [])) != 1:
        sys.exit(f"ELF symbol missing or ambiguous: {name}")
Path(sys.argv[2]).write_text("".join(f"set {n} 0x{found[n][0]}\n" for n in names))
PY
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c init -c 'reset run' -c 'after 4000' \
    -f "$SYMBOLS" -f tools/tasks_probe.tcl -c exit
