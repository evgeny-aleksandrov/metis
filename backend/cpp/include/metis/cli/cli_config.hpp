#pragma once

#include "metis/types.hpp"

#include <string>

namespace metis {

struct CliConfig {
  std::string csv_path = "data/SPY.csv";
  std::string output_path = "ui/public/latest.json";
  std::string symbol = "SPY";
  std::string mode = "grid";
  StrategyType strategy = StrategyType::Drop;
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

}  // namespace metis
