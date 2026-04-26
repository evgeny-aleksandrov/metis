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
  Gain
};

// Strategy knobs (input parameters for one simulation run).
struct StrategyParams {
  StrategyType strategy = StrategyType::Drop;
  double diff_pct = 0.03;
  int lookback_days = 5;
  int hold_days = 10;
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
  double entry_price = 0.0;
  double exit_price = 0.0;
  double shares = 0.0;
  double pnl = 0.0;
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
  int hold_days_min = 10;
  int hold_days_max = 60;
  int hold_days_step = 5;
};

// Reads CSV -> vector of Candle rows.
std::vector<Candle> load_prices_from_csv(const std::string& path);
// Runs one backtest for one parameter set.
SimulationResult run_simulation(const std::vector<Candle>& prices, const StrategyParams& params, double initial_equity);
// Runs many backtests over parameter ranges.
std::vector<SimulationResult> run_grid_search(const std::vector<Candle>& prices, const GridSearchConfig& config, StrategyType strategy, double initial_equity);
StrategyType strategy_type_from_string(const std::string& value);
std::string strategy_type_to_string(StrategyType strategy);
