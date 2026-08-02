#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build/overhead-off" -DIP_ENABLE=OFF -DIP_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/overhead-off" --target workload

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build/overhead-on" -DIP_ENABLE=ON -DIP_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build "$ROOT_DIR/build/overhead-on" --target workload

printf '=== IP_ENABLE=OFF ===\n'
"$ROOT_DIR/build/overhead-off/bench/workload"

printf '\n=== IP_ENABLE=ON ===\n'
"$ROOT_DIR/build/overhead-on/bench/workload"
