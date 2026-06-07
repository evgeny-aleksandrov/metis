#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <string>

namespace metis {

struct CliConfig {
  ApproachType approach = ApproachType::DiscreteGrid;
  StrategyFamily strategy = StrategyFamily::Drop;
  std::string csv_path = "data/SPY.csv";
  std::string output_path = "ui/public/latest.json";
  std::string symbol = "SPY";
  double initial_equity = 10000.0;
  TransactionCosts costs;
  Annualization annualization;
  int walk_train_months = 24;
  int walk_test_months = 6;
  std::string training_results_dir = "outputs/backtests/training-results";
  GridSearchConfig grid;
};

void print_help();
CliConfig parse_args(int argc, char** argv);
BacktestRunConfig to_run_config(const CliConfig& options);

}  // namespace metis
