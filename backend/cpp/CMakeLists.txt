cmake_minimum_required(VERSION 3.16)
project(metis_backtester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(metis_backtester
  src/main.cpp
  src/app/backtester_workflow.cpp
  src/analytics/indicators.cpp
  src/app/backtester_app.cpp
  src/app/approach/buy_and_hold_runner.cpp
  src/app/approach/discrete_grid_runner.cpp
  src/app/approach/ridge_runner.cpp
  src/backtest/metrics.cpp
  src/backtest/buy_and_hold_simulation.cpp
  src/backtest/execution_accounting.cpp
  src/backtest/simulation.cpp
  src/backtest/simulation_rules.cpp
  src/backtest/trade_exit_policy.cpp
  src/backtest/training_candidate_selector.cpp
  src/backtest/walk_forward.cpp
  src/backtest/walk_forward_splitter.cpp
  src/cli/approach/buy_and_hold_cli.cpp
  src/cli/approach/discrete_grid_cli.cpp
  src/cli/approach/ridge_cli.cpp
  src/cli/approach.cpp
  src/cli/cli_config.cpp
  src/cli/help.cpp
  src/cli/parse_common.cpp
  src/config/run_config.cpp
  src/core/date.cpp
  src/core/file_io.cpp
  src/data/csv_loader.cpp
  src/model/ridge_regression.cpp
  src/optimization/discrete_grid_search.cpp
  src/reporting/console_summary.cpp
  src/reporting/json_writer.cpp
  src/reporting/training_results_writer.cpp
  src/strategy/strategy.cpp
  src/strategy/strategy_type.cpp
)

target_include_directories(metis_backtester PRIVATE include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_options(metis_backtester PRIVATE -static-libstdc++ -static-libgcc)
endif()
