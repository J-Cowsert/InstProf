#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT_DIR/test" -B "$ROOT_DIR/test/build-off" -DIP_ENABLE=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/test/build-off" --target workload

cmake -S "$ROOT_DIR/test" -B "$ROOT_DIR/test/build-on" -DIP_ENABLE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/test/build-on" --target workload

printf '=== IP_ENABLE=OFF ===\n'
"$ROOT_DIR/test/build-off/workload"

printf '\n=== IP_ENABLE=ON ===\n'
"$ROOT_DIR/test/build-on/workload"
