#pragma once

#include "metis/types.hpp"

#include <string>
#include <variant>

namespace metis {

enum class BuyAndHoldStrategy {
  LumpSum,
  ScheduledBuy
};

enum class RidgeStrategy {
  Directional,
  LongShort,
  VolTarget
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

template <typename T>
struct Range {
  T min = {};
  T max = {};
  T step = {};
};

struct SearchSettings {
  double sample_pct = 1.0;
  unsigned int seed = 42;
  int random_samples = 0;
};

struct BuyAndHoldSearchSpace {};

struct DiscreteGridSearchSpace {
  Range<double> threshold_pct{0.01, 0.10, 0.01};
  Range<int> lookback_days{2, 20, 1};
  Range<int> fast_lookback_days{2, 10, 1};
  Range<int> short_lookback_days{0, 0, 1};
  Range<int> short_fast_lookback_days{0, 0, 1};
  Range<int> hold_days{10, 60, 5};
  Range<double> take_profit_pct{0.0, 0.0, 0.01};
  Range<double> stop_loss_pct{0.0, 0.0, 0.01};
  Range<double> trailing_stop_pct{0.0, 0.0, 0.01};
  Range<int> regime_weak_exit{0, 0, 1};
  Range<int> allow_short{0, 0, 1};
  Range<int> volatility_lookback_days{0, 0, 1};
  Range<double> target_volatility{0.0, 0.0, 0.01};
  Range<double> max_position_pct{1.0, 1.0, 0.25};
};

struct RidgeSearchSpace {
  Range<double> alpha{0.0, 0.0, 0.1};
  Range<int> feature_lookback_days{1, 1, 1};
  Range<int> target_horizon_days{1, 1, 1};
  Range<double> signal_threshold{0.0, 0.0, 0.01};
};

struct BuyAndHoldRunConfig {
  BuyAndHoldStrategy strategy = BuyAndHoldStrategy::LumpSum;
  BuyAndHoldSearchSpace search_space;
  SearchSettings search;
};

struct DiscreteGridRunConfig {
  DiscreteGridStrategy strategy = DiscreteGridStrategy::Drop;
  DiscreteGridSearchSpace search_space;
  SearchSettings search;
};

struct RidgeRunConfig {
  RidgeStrategy strategy = RidgeStrategy::Directional;
  RidgeSearchSpace search_space;
  SearchSettings search;
};

using ApproachRunConfig = std::variant<BuyAndHoldRunConfig, DiscreteGridRunConfig, RidgeRunConfig>;

struct BacktestRunConfig {
  DataConfig data;
  OutputConfig output;
  ExecutionConfig execution;
  WalkForwardConfig walk_forward;
  ApproachRunConfig approach_config = DiscreteGridRunConfig{};
};

std::string approach_name(const BacktestRunConfig& config);
std::string strategy_name(const BacktestRunConfig& config);

}  // namespace metis
