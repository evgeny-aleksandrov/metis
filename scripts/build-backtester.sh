#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"

if command -v cmake >/dev/null 2>&1; then
  cmake -S "$repo_root/backend/cpp" -B "$repo_root/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$repo_root/build" --target metis_backtester
else
  mkdir -p "$repo_root/build"
  compiler="clang++"
  if command -v xcrun >/dev/null 2>&1; then
    compiler="xcrun clang++"
  fi
  $compiler \
    -std=c++17 \
    -O2 \
    -I"$repo_root/backend/cpp/include" \
    "$repo_root/backend/cpp/src/main.cpp" \
    "$repo_root/backend/cpp/src/analytics/indicators.cpp" \
    "$repo_root/backend/cpp/src/app/backtester_app.cpp" \
    "$repo_root/backend/cpp/src/backtest/backtest.cpp" \
    "$repo_root/backend/cpp/src/backtest/metrics.cpp" \
    "$repo_root/backend/cpp/src/backtest/walk_forward.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/buy_and_hold_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/discrete_grid_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/ridge_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach.cpp" \
    "$repo_root/backend/cpp/src/cli/cli_config.cpp" \
    "$repo_root/backend/cpp/src/cli/help.cpp" \
    "$repo_root/backend/cpp/src/cli/parse_common.cpp" \
    "$repo_root/backend/cpp/src/core/date.cpp" \
    "$repo_root/backend/cpp/src/core/file_io.cpp" \
    "$repo_root/backend/cpp/src/data/csv_loader.cpp" \
    "$repo_root/backend/cpp/src/model/ridge_regression.cpp" \
    "$repo_root/backend/cpp/src/reporting/console_summary.cpp" \
    "$repo_root/backend/cpp/src/reporting/json_writer.cpp" \
    "$repo_root/backend/cpp/src/reporting/training_results_writer.cpp" \
    "$repo_root/backend/cpp/src/strategy/strategy.cpp" \
    "$repo_root/backend/cpp/src/strategy/strategy_type.cpp" \
    -o "$repo_root/build/metis_backtester"
fi
