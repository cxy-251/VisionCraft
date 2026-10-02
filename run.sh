#!/bin/bash
# Steam Deck 上编译并运行 VisionCraft。
#   ./run.sh           没有构建产物时先编译，然后运行
#   ./run.sh --build   只编译
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"
source scripts/steamdeck-env.sh

if [ "$1" = "--build" ] || [ ! -f build/VisionCraft ]; then
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_SYSROOT="$VC_SYSROOT"
    cmake --build build --parallel
    [ "$1" = "--build" ] && exit 0
fi

exec ./build/VisionCraft "$@"
