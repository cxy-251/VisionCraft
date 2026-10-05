#!/bin/bash
# 修正 ST USB 主机库 HID 类里 SET_PROTOCOL 的取值（CubeMX 每次生成都会把这个文件恢复原样，所以生成后要重新打）。
#   tools/patch_usbh_hid.sh [firmware/station]
# HID 规范：wValue 0 = 引导协议，1 = 报告协议。库里 USBH_HID_SetProtocol(phost, 0) 想要引导协议，却发出 1。
# 见手册「指针事件：触摸和鼠标」。已经打过就什么也不做。
set -e
F="${1:-$(dirname "$0")/../firmware/station}/Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c"
if grep -q "VisionCraft 修改" "$F"; then echo "已修正：$F"; exit 0; fi
# [region patch]
python3 - "$F" <<'PY'
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8", newline="").read()
nl = "\r\n" if "\r\n" in s else "\n"
old = nl.join(["  if (protocol != 0U)", "  {", "    phost->Control.setup.b.wValue.w = 0U;", "  }", "  else", "  {",
               "    phost->Control.setup.b.wValue.w = 1U;", "  }"])
new = nl.join(["  /* VisionCraft 修改：HID 规范里 wValue 0 = 引导协议，1 = 报告协议。原代码把它反过来了：",
               "   * 调用方传 0（想要引导协议）却发出 1，鼠标于是按自己的报告格式发数据，",
               "   * 而 usbh_hid_mouse.c 按引导格式（按键、X、Y 各 1 字节）解析，X、Y 全错。",
               "   * 见手册「指针事件：触摸和鼠标」。CubeMX 重新生成会覆盖本文件，tools/patch_usbh_hid.sh 会重新打上 */",
               "  phost->Control.setup.b.wValue.w = (protocol != 0U) ? 1U : 0U;"])
if s.count(old) != 1:
    sys.exit("没找到要改的代码，库的版本可能变了：" + p)
open(p, "w", encoding="utf-8", newline="").write(s.replace(old, new))
PY
# [endregion]
echo "已修正：$F"
