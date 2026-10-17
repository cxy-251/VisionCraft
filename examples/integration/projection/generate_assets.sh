#!/usr/bin/env bash
set -euo pipefail
vc_root=$(cd "$(dirname "$0")/../../.." && pwd)
"$vc_root/build/examples/integration/projection/example_projection_lesson" "$vc_root/handbook/assets/projection" > "$vc_root/examples/integration/projection/output.txt"
