# 第三方源码不放进仓库：配置（cmake）时按固定版本从 GitHub 拉下来，放到 CubeMX 生成的工程期望的位置
# （这些目录都在 .gitignore 里）。已经有了就跳过，所以只有第一次配置要联网。
#   LVGL            → firmware/third_party/lvgl
#   ST 的 HAL、CMSIS、FreeRTOS、USB 主机库 → firmware/station/Drivers、Middlewares（清单见 stm32cubef4_files.txt）
# 固件（firmware/station/CMakeLists.txt）和上位机（根目录 CMakeLists.txt：模拟器要 LVGL，手册要引用几个 ST 文件）都会调用。
include_guard(GLOBAL)

set(VC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE INTERNAL "")
get_filename_component(VC_ROOT "${VC_ROOT}" ABSOLUTE)

# [region versions]
set(VC_LVGL_TAG     v9.6.0)                                     # https://github.com/lvgl/lvgl
set(VC_CUBEF4_TAG   v1.28.3)                                    # https://github.com/STMicroelectronics/STM32CubeF4
# STM32CubeF4 里这两个目录是子模块，按 v1.28.3 记录的提交拉
set(VC_HAL_COMMIT   b6f0ed3829f3829eb358a2e7417d80bba1a42db7)   # stm32f4xx_hal_driver
set(VC_CMSIS_COMMIT 3c77349ce04c8af401454cc51f85ea9a50e34fc1)   # cmsis_device_f4
# [endregion]

find_package(Git REQUIRED)

# 访问 GitHub 偶尔会断（实测第一次克隆 LVGL 卡了近 10 分钟后失败，重试 25 秒成功），所以失败了重试，最多 3 次
function(_vc_git)
    foreach(attempt 1 2 3)
        execute_process(COMMAND "${GIT_EXECUTABLE}" ${ARGN} RESULT_VARIABLE rc OUTPUT_QUIET ERROR_VARIABLE err
                        TIMEOUT 600)
        if(rc EQUAL 0)
            return()
        endif()
        message(STATUS "git ${ARGN} 第 ${attempt} 次失败：${err}")
    endforeach()
    message(FATAL_ERROR "git ${ARGN} 失败了 3 次，检查网络后重新运行 cmake")
endfunction()

# 只拉一个提交、只检出需要的目录：LVGL 整个仓库约 300 MB（文档、示例、测试），用到的只有 src 和 include
function(_vc_sparse_clone url ref dir)
    file(REMOVE_RECURSE "${dir}")
    _vc_git(clone --quiet --depth 1 --filter=blob:none --sparse --branch ${ref} ${url} "${dir}")
    _vc_git(-C "${dir}" sparse-checkout set ${ARGN})
endfunction()

function(_vc_fetch_commit url commit dir)
    file(REMOVE_RECURSE "${dir}")
    file(MAKE_DIRECTORY "${dir}")
    _vc_git(-C "${dir}" init --quiet)
    _vc_git(-C "${dir}" fetch --quiet --depth 1 ${url} ${commit})
    _vc_git(-C "${dir}" checkout --quiet FETCH_HEAD)
endfunction()

# [region lvgl]
function(vc_fetch_lvgl)
    set(dest "${VC_ROOT}/firmware/third_party/lvgl")
    if(EXISTS "${dest}/lvgl.h")
        return()
    endif()
    message(STATUS "拉取 LVGL ${VC_LVGL_TAG}（只要 src、include）…")
    set(tmp "${VC_ROOT}/firmware/third_party/.fetch/lvgl")
    _vc_sparse_clone(https://github.com/lvgl/lvgl.git ${VC_LVGL_TAG} "${tmp}" src include)
    file(MAKE_DIRECTORY "${dest}")
    file(COPY "${tmp}/src" "${tmp}/include" "${tmp}/lvgl.h" "${tmp}/lvgl_private.h" "${tmp}/lv_version.h"
              "${tmp}/LICENCE.txt" "${tmp}/README.md" DESTINATION "${dest}")
    file(REMOVE_RECURSE "${tmp}")
endfunction()
# [endregion]

# [region stm32]
function(vc_fetch_stm32cube)
    set(station "${VC_ROOT}/firmware/station")
    file(STRINGS "${VC_ROOT}/firmware/third_party/stm32cubef4_files.txt" files ENCODING UTF-8 REGEX "^[^#]")
    set(missing "")
    foreach(f IN LISTS files)
        if(NOT EXISTS "${station}/${f}")
            list(APPEND missing "${f}")
        endif()
    endforeach()
    if(NOT missing)
        return()
    endif()
    list(LENGTH missing n)
    message(STATUS "拉取 STM32CubeF4 ${VC_CUBEF4_TAG}（缺 ${n} 个文件）…")
    set(tmp "${VC_ROOT}/firmware/third_party/.fetch")
    _vc_sparse_clone(https://github.com/STMicroelectronics/STM32CubeF4.git ${VC_CUBEF4_TAG} "${tmp}/cube"
                     Drivers/CMSIS/Include Middlewares/ST/STM32_USB_Host_Library Middlewares/Third_Party/FreeRTOS)
    _vc_fetch_commit(https://github.com/STMicroelectronics/stm32f4xx_hal_driver.git ${VC_HAL_COMMIT} "${tmp}/hal")
    _vc_fetch_commit(https://github.com/STMicroelectronics/cmsis_device_f4.git ${VC_CMSIS_COMMIT} "${tmp}/cmsis_f4")
    foreach(f IN LISTS missing)
        if(f MATCHES "^Drivers/STM32F4xx_HAL_Driver/(.*)")
            set(src "${tmp}/hal/${CMAKE_MATCH_1}")
        elseif(f MATCHES "^Drivers/CMSIS/Device/ST/STM32F4xx/(.*)")
            set(src "${tmp}/cmsis_f4/${CMAKE_MATCH_1}")
        else()
            set(src "${tmp}/cube/${f}")
        endif()
        if(NOT EXISTS "${src}")
            message(FATAL_ERROR "上游没有这个文件：${f}")
        endif()
        get_filename_component(dir "${station}/${f}" DIRECTORY)
        file(MAKE_DIRECTORY "${dir}")
        file(COPY_FILE "${src}" "${station}/${f}")
    endforeach()
    file(REMOVE_RECURSE "${tmp}")
    # USB 主机库的那处 bug，拉下来就修好（见 patch_usbh_hid.cmake）
    execute_process(COMMAND "${CMAKE_COMMAND}"
        -DFILE=${station}/Middlewares/ST/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c
        -P "${VC_ROOT}/firmware/third_party/patch_usbh_hid.cmake" RESULT_VARIABLE rc)
    if(rc)
        message(FATAL_ERROR "usbh_hid.c 打补丁失败")
    endif()
endfunction()
# [endregion]
