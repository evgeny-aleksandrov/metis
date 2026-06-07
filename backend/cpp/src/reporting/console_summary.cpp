#include "metis/reporting/console_summary.hpp"

#include "metis/strategy/strategy_type.hpp"

#include <iomanip>
#include <iostream>

namespace metis {

void print_summary(
    const CliConfig& config,
    size_t rows_loaded,
    const SimulationResult& buy_and_hold,
    const SimulationResult& summary,
    size_t walk_periods) {
  std::cout << "Backtest complete.\n";
  std::cout << "Symbol: " << config.symbol << "\n";
  std::cout << "Mode: " << config.mode << "\n";
  std::cout << "Strategy: " << strategy_type_to_string(config.strategy) << "\n";
  std::cout << "Rows loaded: " << rows_loaded << "\n";
  std::cout << "Fixed cost/order: " << config.costs.fixed_per_order << "\n";
  std::cout << "Variable cost rate/order: " << config.costs.variable_rate << "\n";
  std::cout << "Bars/year: " << config.annualization.bars_per_year << "\n";
  std::cout << "Buy-and-hold CAGR: " << std::fixed << std::setprecision(4) << (buy_and_hold.metrics.cagr * 100.0) << "%\n";
  if (config.mode == "walk-forward") {
    std::cout << "Walk-forward periods: " << walk_periods << "\n";
    std::cout << "Train months: " << config.walk_train_months << "\n";
    std::cout << "Test months: " << config.walk_test_months << "\n";
    std::cout << "Out-of-sample CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
  } else {
    std::cout << "Best threshold_pct: " << summary.params.diff_pct << "\n";
    std::cout << "Best lookback_days: " << summary.params.lookback_days << "\n";
    std::cout << "Best fast_lookback_days: " << summary.params.fast_lookback_days << "\n";
    std::cout << "Best hold_days: " << summary.params.hold_days << "\n";
    std::cout << "Best CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
  }
  std::cout << "Output written: " << config.output_path << "\n";
}

}  // namespace metis
