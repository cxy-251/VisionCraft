#!/bin/bash
# 修正 ST USB 主机库 HID 类里 SET_PROTOCOL 的取值（CubeMX 每次生成都会把这个文件恢复原样，所以生成后要重新打）。
#   tools/patch_usbh_hid.sh [firmware/station]
# 改法写在 firmware/third_party/patch_usbh_hid.cmake 里（拉取第三方源码时也用它），这里只是转调。
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
F="${1:-$ROOT/firmware/station}/Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c"
command -v cmake >/dev/null || . "$ROOT/scripts/steamdeck-env.sh"
cmake -DFILE="$F" -P "$ROOT/firmware/third_party/patch_usbh_hid.cmake"
