#pragma once

#include "metis/cli/approach.hpp"
#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <string>
#include <variant>

namespace metis {

struct BuyAndHoldCliOptions {
  BuyAndHoldSearchSpace search_space;
  SearchSettings search;
};

struct DiscreteGridCliOptions {
  DiscreteGridSearchSpace search_space;
  SearchSettings search;
};

struct RidgeCliOptions {
  RidgeSearchSpace search_space;
  SearchSettings search;
};

using ApproachCliOptions = std::variant<BuyAndHoldCliOptions, DiscreteGridCliOptions, RidgeCliOptions>;

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
  ApproachCliOptions approach_options = DiscreteGridCliOptions{};
};

void print_help();
CliConfig parse_args(int argc, char** argv);
BacktestRunConfig to_run_config(const CliConfig& options);

}  // namespace metis
