#!/bin/bash
# Steam Deck (SteamOS 只读根分区) 上的开发环境。
# 每个工具单独解压在 ~/Applications/<工具名>/usr 下（Arch Linux 软件包）。
# 用法: source scripts/steamdeck-env.sh

# [region path]
VC_TOOLS="${VC_TOOLS:-$HOME/Applications}"

for _t in gcc cmake ninja qt6 arm-none-eabi openocd picocom; do
    PATH="$VC_TOOLS/$_t/usr/bin:$PATH"
done
PATH="$VC_TOOLS/qt6/usr/lib/qt6/bin:$PATH"
export PATH
# [endregion]

# [region libpath]
# 只放运行时需要、系统里没有的库。gcc/usr/lib 里有一份 glibc，不能整体加进来；
# 编译器只缺 isl 和 mpc，单独链接在 gcc/runtime-libs。
_libs="$VC_TOOLS/gcc/runtime-libs"
for _t in cmake qt6 opencv openocd fmt; do
    _libs="$_libs:$VC_TOOLS/$_t/usr/lib"
done
export LD_LIBRARY_PATH="$_libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# [endregion]

# [region cmake]
export CMAKE_PREFIX_PATH="$VC_TOOLS/qt6/usr:$VC_TOOLS/opencv/usr:$VC_TOOLS/fmt/usr:$VC_TOOLS/nlohmann-json/usr"
export OpenCV_DIR="$VC_TOOLS/opencv/usr/lib/cmake/opencv4"
export VC_SYSROOT="$VC_TOOLS/gcc"
export QT_PLUGIN_PATH="$VC_TOOLS/qt6/usr/lib/qt6/plugins"
# 本机工具目录未包含Wayland/FFmpeg插件时，可使用系统同版本Qt插件。
# 仅在Qt Core精确版本一致时加入，避免不同Qt版本的插件混装。
if [ -f "$VC_TOOLS/qt6/usr/lib/libQt6Core.so.6" ] && [ -f /usr/lib/libQt6Core.so.6 ] && \
   [ "$(basename "$(readlink -f "$VC_TOOLS/qt6/usr/lib/libQt6Core.so.6")")" = \
     "$(basename "$(readlink -f /usr/lib/libQt6Core.so.6)")" ] && \
   [ -d /usr/lib/qt6/plugins ]; then
    export QT_PLUGIN_PATH="$QT_PLUGIN_PATH:/usr/lib/qt6/plugins"
fi
export QML_IMPORT_PATH="$VC_TOOLS/qt6/usr/lib/qt6/qml"
# [endregion]

# [region rpath-link]
# Qt6Network -> 系统 libproxy -> /usr/lib/libproxy/libpxbackend-1.0.so。libproxy 自带 RUNPATH /usr/lib/libproxy，
# Qt Multimedia同样经libpulse引用/usr/lib/pulseaudio中的libpulsecommon。
# 用了--sysroot后，链接器会把sysroot加在RUNPATH前面；显式提供两处间接依赖目录。
export LDFLAGS="-Wl,-rpath-link,/usr/lib/libproxy -Wl,-rpath-link,/usr/lib/pulseaudio${LDFLAGS:+ $LDFLAGS}"
# [endregion]

unset _t _libs
