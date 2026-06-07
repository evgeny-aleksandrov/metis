cmake_minimum_required(VERSION 3.16)
project(metis_backtester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(metis_backtester
  src/main.cpp
  src/analytics/indicators.cpp
  src/app/backtester_app.cpp
  src/backtest/backtest.cpp
  src/backtest/metrics.cpp
  src/backtest/walk_forward.cpp
  src/cli/approach/buy_and_hold_cli.cpp
  src/cli/approach/discrete_grid_cli.cpp
  src/cli/approach/ridge_cli.cpp
  src/cli/approach.cpp
  src/cli/cli_config.cpp
  src/cli/help.cpp
  src/cli/parse_common.cpp
  src/core/date.cpp
  src/core/file_io.cpp
  src/data/csv_loader.cpp
  src/model/ridge_regression.cpp
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
