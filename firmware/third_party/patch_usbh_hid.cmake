# 修正 ST USB 主机库 HID 类里 SET_PROTOCOL 的取值。用法：cmake -DFILE=<usbh_hid.c 的路径> -P patch_usbh_hid.cmake
# HID 规范：wValue 0 = 引导协议，1 = 报告协议。库里 USBH_HID_SetProtocol(phost, 0) 想要引导协议，却发出 1。
# 见手册「指针事件：触摸和鼠标」。已经打过就什么也不做。由 fetch.cmake（拉取后）和 tools/patch_usbh_hid.sh（CubeMX 生成后）调用。
if(NOT FILE)
    message(FATAL_ERROR "用法：cmake -DFILE=<usbh_hid.c> -P patch_usbh_hid.cmake")
endif()
file(READ "${FILE}" s)
if(s MATCHES "VisionCraft 修改")
    message(STATUS "已修正：${FILE}")
    return()
endif()
# [region patch]
if(s MATCHES "\r\n")                       # 保持文件原来的换行方式
    set(nl "\r\n")
else()
    set(nl "\n")
endif()
string(JOIN "${nl}" old
    "  if (protocol != 0U)" "  {" "    phost->Control.setup.b.wValue.w = 0U;" "  }" "  else" "  {"
    "    phost->Control.setup.b.wValue.w = 1U;" "  }")
string(JOIN "${nl}" new
    "  /* VisionCraft 修改：HID 规范里 wValue 0 = 引导协议，1 = 报告协议。原代码把它反过来了："
    "   * 调用方传 0（想要引导协议）却发出 1，鼠标于是按自己的报告格式发数据，"
    "   * 而 usbh_hid_mouse.c 按引导格式（按键、X、Y 各 1 字节）解析，X、Y 全错。"
    "   * 见手册「指针事件：触摸和鼠标」。CubeMX 重新生成、或重新拉取时都会恢复原样，由 patch_usbh_hid.cmake 重新打上 */"
    "  phost->Control.setup.b.wValue.w = (protocol != 0U) ? 1U : 0U;")
string(FIND "${s}" "${old}" pos)
if(pos EQUAL -1)
    message(FATAL_ERROR "没找到要改的代码，库的版本可能变了：${FILE}")
endif()
string(REPLACE "${old}" "${new}" s "${s}")
file(WRITE "${FILE}" "${s}")
# [endregion]
message(STATUS "已修正：${FILE}")
