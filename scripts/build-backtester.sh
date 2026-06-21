#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"
build_dir="${METIS_BUILD_DIR:-$repo_root/build}"

if command -v cmake >/dev/null 2>&1; then
  cmake -S "$repo_root/backend/cpp" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$build_dir" --target metis_backtester
else
  mkdir -p "$build_dir"
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
    "$repo_root/backend/cpp/src/app/backtester_workflow.cpp" \
    "$repo_root/backend/cpp/src/app/backtester_app.cpp" \
    "$repo_root/backend/cpp/src/app/approach/buy_and_hold_runner.cpp" \
    "$repo_root/backend/cpp/src/app/approach/discrete_grid_runner.cpp" \
    "$repo_root/backend/cpp/src/app/approach/ridge_runner.cpp" \
    "$repo_root/backend/cpp/src/backtest/metrics.cpp" \
    "$repo_root/backend/cpp/src/backtest/buy_and_hold_simulation.cpp" \
    "$repo_root/backend/cpp/src/backtest/execution_accounting.cpp" \
    "$repo_root/backend/cpp/src/backtest/position_sizing_policy.cpp" \
    "$repo_root/backend/cpp/src/backtest/simulation.cpp" \
    "$repo_root/backend/cpp/src/backtest/trade_exit_policy.cpp" \
    "$repo_root/backend/cpp/src/backtest/training_candidate_selector.cpp" \
    "$repo_root/backend/cpp/src/backtest/walk_forward.cpp" \
    "$repo_root/backend/cpp/src/backtest/walk_forward_splitter.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/buy_and_hold_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/discrete_grid_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach/ridge_cli.cpp" \
    "$repo_root/backend/cpp/src/cli/approach.cpp" \
    "$repo_root/backend/cpp/src/cli/cli_config.cpp" \
    "$repo_root/backend/cpp/src/cli/help.cpp" \
    "$repo_root/backend/cpp/src/cli/parse_common.cpp" \
    "$repo_root/backend/cpp/src/config/run_config.cpp" \
    "$repo_root/backend/cpp/src/core/date.cpp" \
    "$repo_root/backend/cpp/src/core/file_io.cpp" \
    "$repo_root/backend/cpp/src/data/csv_loader.cpp" \
    "$repo_root/backend/cpp/src/model/ridge_regression.cpp" \
    "$repo_root/backend/cpp/src/optimization/discrete_grid_search.cpp" \
    "$repo_root/backend/cpp/src/reporting/console_summary.cpp" \
    "$repo_root/backend/cpp/src/reporting/json_writer.cpp" \
    "$repo_root/backend/cpp/src/reporting/training_results_writer.cpp" \
    "$repo_root/backend/cpp/src/strategy/regime_strategy.cpp" \
    "$repo_root/backend/cpp/src/strategy/regime_strategy_rules.cpp" \
    "$repo_root/backend/cpp/src/strategy/ridge_strategy.cpp" \
    "$repo_root/backend/cpp/src/strategy/ridge_strategy_rules.cpp" \
    "$repo_root/backend/cpp/src/strategy/strategy_factory.cpp" \
    "$repo_root/backend/cpp/src/strategy/strategy_rules_factory.cpp" \
    "$repo_root/backend/cpp/src/strategy/strategy_type.cpp" \
    "$repo_root/backend/cpp/src/strategy/threshold_strategy.cpp" \
    "$repo_root/backend/cpp/src/strategy/threshold_strategy_rules.cpp" \
    -o "$build_dir/metis_backtester"
fi
