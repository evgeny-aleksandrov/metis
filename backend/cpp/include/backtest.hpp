#pragma once

#include <string>
#include <vector>

// Simple "data class" (similar to a Java POJO / Python dataclass).
// In C++, `struct` members are public by default.
struct Candle {
  std::string date;
  double close = 0.0;
};

enum class StrategyType {
  Drop,
  Gain,
  Regime
};

// Strategy knobs (input parameters for one simulation run).
struct StrategyParams {
  StrategyType strategy = StrategyType::Drop;
  double diff_pct = 0.03;
  int lookback_days = 5;
  int fast_lookback_days = 2;
  int short_lookback_days = 5;
  int short_fast_lookback_days = 2;
  int hold_days = 10;
  double take_profit_pct = 0.0;
  double stop_loss_pct = 0.0;
  double trailing_stop_pct = 0.0;
  bool exit_on_regime_weakness = false;
  bool allow_short = false;
  int volatility_lookback_days = 0;
  double target_volatility = 0.0;
  double max_position_pct = 1.0;
};

struct Annualization {
  double bars_per_year = 252.0;
};

struct TransactionCosts {
  double fixed_per_order = 0.0;
  double variable_rate = 0.0;
};

// Performance outputs produced by one simulation.
struct Metrics {
  double total_return = 0.0;
  double cagr = 0.0;
  double max_drawdown = 0.0;
  double sharpe = 0.0;
  double sortino = 0.0;
  int trades = 0;
  double win_rate = 0.0;
};

struct EquityPoint {
  std::string date;
  double equity = 0.0;
};

struct Trade {
  std::string entry_date;
  std::string exit_date;
  std::string direction = "long";
  double entry_price = 0.0;
  double exit_price = 0.0;
  double shares = 0.0;
  double pnl = 0.0;
  double costs = 0.0;
  double entry_signal_quality = 0.0;
  int holding_days = 0;
};

// Combines params + metrics + ending portfolio value.
struct SimulationResult {
  StrategyParams params;
  Metrics metrics;
  double final_equity = 0.0;
  std::vector<EquityPoint> equity_curve;
  std::vector<Trade> trades;
};

// Grid-search ranges. Think nested loops over x and y.
struct GridSearchConfig {
  double x_min = 0.01;
  double x_max = 0.10;
  double x_step = 0.01;
  int y_min = 2;
  int y_max = 20;
  int y_step = 1;
  int fast_lookback_min = 2;
  int fast_lookback_max = 10;
  int fast_lookback_step = 1;
  int short_y_min = 0;
  int short_y_max = 0;
  int short_y_step = 1;
  int short_fast_lookback_min = 0;
  int short_fast_lookback_max = 0;
  int short_fast_lookback_step = 1;
  int hold_days_min = 10;
  int hold_days_max = 60;
  int hold_days_step = 5;
  double take_profit_min = 0.0;
  double take_profit_max = 0.0;
  double take_profit_step = 0.01;
  double stop_loss_min = 0.0;
  double stop_loss_max = 0.0;
  double stop_loss_step = 0.01;
  double trailing_stop_min = 0.0;
  double trailing_stop_max = 0.0;
  double trailing_stop_step = 0.01;
  int regime_weak_exit_min = 0;
  int regime_weak_exit_max = 0;
  int allow_short_min = 0;
  int allow_short_max = 0;
  int volatility_lookback_min = 0;
  int volatility_lookback_max = 0;
  int volatility_lookback_step = 1;
  double target_volatility_min = 0.0;
  double target_volatility_max = 0.0;
  double target_volatility_step = 0.01;
  double max_position_pct_min = 1.0;
  double max_position_pct_max = 1.0;
  double max_position_pct_step = 0.25;
  double grid_sample_pct = 1.0;
  unsigned int grid_sample_seed = 42;
  int grid_random_samples = 0;
};

// Reads CSV -> vector of Candle rows.
std::vector<Candle> load_prices_from_csv(const std::string& path);
// Runs one backtest for one parameter set.
SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const StrategyParams& params,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});
// Runs a fully invested buy-and-hold benchmark over the same data.
SimulationResult run_buy_and_hold(
    const std::vector<Candle>& prices,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});
// Runs a one-position majority-vote ensemble from ranked parameter sets.
SimulationResult run_ensemble_simulation(
    const std::vector<Candle>& prices,
    const std::vector<StrategyParams>& params,
    bool rank_weighted,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});
// Runs many backtests over parameter ranges.
std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});
void sort_results_by_cagr(std::vector<SimulationResult>& results);
StrategyType strategy_type_from_string(const std::string& value);
std::string strategy_type_to_string(StrategyType strategy);
