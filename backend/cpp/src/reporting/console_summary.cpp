#include "metis/reporting/console_summary.hpp"

#include "metis/cli/approach.hpp"
#include "metis/strategy/strategy_type.hpp"

#include <iomanip>
#include <iostream>

namespace metis {

void print_summary(
    const BacktestRunConfig& config,
    size_t rows_loaded,
    const SimulationResult& buy_and_hold,
    const SimulationResult& summary,
    size_t walk_periods) {
  std::cout << "Backtest complete.\n";
  std::cout << "Symbol: " << config.data.symbol << "\n";
  std::cout << "Approach: " << approach_type_to_string(config.approach) << "\n";
  std::cout << "Strategy: " << strategy_family_to_string(config.strategy) << "\n";
  std::cout << "Rows loaded: " << rows_loaded << "\n";
  std::cout << "Fixed cost/order: " << config.execution.costs.fixed_per_order << "\n";
  std::cout << "Variable cost rate/order: " << config.execution.costs.variable_rate << "\n";
  std::cout << "Bars/year: " << config.execution.annualization.bars_per_year << "\n";
  std::cout << "Buy-and-hold CAGR: " << std::fixed << std::setprecision(4) << (buy_and_hold.metrics.cagr * 100.0) << "%\n";
  if (walk_periods > 0) {
    std::cout << "Walk-forward periods: " << walk_periods << "\n";
    std::cout << "Train months: " << config.walk_forward.train_months << "\n";
    std::cout << "Test months: " << config.walk_forward.test_months << "\n";
    std::cout << "Out-of-sample CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
  } else {
    std::cout << "Final CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
  }
  std::cout << "Output written: " << config.output.json_path << "\n";
}

}  // namespace metis
