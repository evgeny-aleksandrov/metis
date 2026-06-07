#pragma once

#include "metis/types.hpp"

#include <string>

namespace metis {

enum class ApproachType {
  BuyAndHold,
  DiscreteGrid,
  Ridge
};

enum class StrategyFamily {
  LumpSumBuyAndHold,
  ScheduledBuy,
  Drop,
  Gain,
  Regime,
  RidgeDirectional,
  RidgeLongShort,
  RidgeVolTarget
};

struct DataConfig {
  std::string csv_path = "data/SPY.csv";
  std::string symbol = "SPY";
};

struct OutputConfig {
  std::string json_path = "ui/public/latest.json";
  std::string training_results_dir = "outputs/backtests/training-results";
};

struct ExecutionConfig {
  double initial_equity = 10000.0;
  TransactionCosts costs;
  Annualization annualization;
};

struct WalkForwardConfig {
  int train_months = 24;
  int test_months = 6;
};

struct DiscreteStrategySearchConfig {
  StrategyType strategy = StrategyType::Drop;
  GridSearchConfig grid;
};

struct BacktestRunConfig {
  ApproachType approach = ApproachType::DiscreteGrid;
  StrategyFamily strategy = StrategyFamily::Drop;
  DataConfig data;
  OutputConfig output;
  ExecutionConfig execution;
  WalkForwardConfig walk_forward;
  DiscreteStrategySearchConfig discrete_search;
};

}  // namespace metis
