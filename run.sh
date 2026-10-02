#!/bin/bash
set -e

DEVKIT_DIR="/home/deck/Applications/devkit"
export PATH="$DEVKIT_DIR/usr/bin:$PATH"
export LD_LIBRARY_PATH="$DEVKIT_DIR/usr/lib:${LD_LIBRARY_PATH:-}"

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$DIR"

if [ "$1" = "--build" ]; then
    cmake -B build -G Ninja \
      -DCMAKE_CXX_COMPILER="$DEVKIT_DIR/usr/bin/g++" \
      -DCMAKE_C_COMPILER="$DEVKIT_DIR/usr/bin/gcc" \
      -DCMAKE_SYSROOT="$DEVKIT_DIR" \
      -DCMAKE_PREFIX_PATH="$DEVKIT_DIR/usr" \
      -DOpenCV_DIR="$DEVKIT_DIR/usr/lib/cmake/opencv4"
    cmake --build build --parallel
    exit 0
fi

if [ ! -f "build/VisionCraft" ]; then
    "$0" --build
fi

exec ./build/VisionCraft "$@"
