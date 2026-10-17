#!/usr/bin/env bash
set -euo pipefail
vc_root=$(cd "$(dirname "$0")/../../.." && pwd)
"$vc_root/build/examples/opencv/multiimage/example_multiimage_lessons" "$vc_root/handbook/assets/multiimage" > "$vc_root/examples/opencv/multiimage/output.txt"
