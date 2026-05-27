#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"

if command -v cmake >/dev/null 2>&1; then
  cmake -S "$repo_root/backend/cpp" -B "$repo_root/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$repo_root/build" --target metis_backtester
else
  mkdir -p "$repo_root/build"
  xcrun clang++ \
    -std=c++17 \
    -O2 \
    -I"$repo_root/backend/cpp/include" \
    "$repo_root/backend/cpp/src/main.cpp" \
    "$repo_root/backend/cpp/src/backtest.cpp" \
    -o "$repo_root/build/metis_backtester"
fi
