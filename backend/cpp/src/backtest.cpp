#include "backtest.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

// Anonymous namespace = file-private helpers (similar to private static helpers in a Java class).
namespace {
// Very small CSV splitter. `std::vector<std::string>` is like `List<String>` in Java.
std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) {
    fields.push_back(field);
  }
  return fields;
}

// Max drawdown = largest peak-to-trough drop in the equity curve.
double compute_max_drawdown(const std::vector<double>& equity_curve) {
  if (equity_curve.empty()) {
    return 0.0;
  }
  double peak = equity_curve.front();
  double max_drawdown = 0.0;
  for (double value : equity_curve) {
    if (value > peak) {
      peak = value;
    }
    if (peak > 0.0) {
      const double drawdown = (peak - value) / peak;
      if (drawdown > max_drawdown) {
        max_drawdown = drawdown;
      }
    }
  }
  return max_drawdown;
}

// Sharpe ratio from daily equity changes.
// `size_t` is an unsigned index type commonly used for container sizes.
double compute_sharpe(const std::vector<double>& equity_curve) {
  if (equity_curve.size() < 2) {
    return 0.0;
  }
  std::vector<double> returns;
  returns.reserve(equity_curve.size() - 1);
  for (size_t index = 1; index < equity_curve.size(); ++index) {
    const double prev = equity_curve[index - 1];
    const double current = equity_curve[index];
    if (prev <= 0.0) {
      returns.push_back(0.0);
    } else {
      returns.push_back((current - prev) / prev);
    }
  }

  double mean = 0.0;
  for (double value : returns) {
    mean += value;
  }
  mean /= static_cast<double>(returns.size());

  double variance = 0.0;
  for (double value : returns) {
    const double diff = value - mean;
    variance += diff * diff;
  }
  variance /= static_cast<double>(returns.size());

  const double stddev = std::sqrt(variance);
  if (stddev == 0.0) {
    return 0.0;
  }

  return (mean / stddev) * std::sqrt(252.0);
}

// CAGR (annualized return) using ~252 trading days/year.
double compute_cagr(double initial_equity, double final_equity, size_t bars) {
  if (initial_equity <= 0.0 || final_equity <= 0.0 || bars < 2) {
    return 0.0;
  }
  const double years = static_cast<double>(bars - 1) / 252.0;
  if (years <= 0.0) {
    return 0.0;
  }
  return std::pow(final_equity / initial_equity, 1.0 / years) - 1.0;
}
}  // namespace

// Parses Yahoo-style CSV and keeps Date + adjusted/close price.
std::vector<Candle> load_prices_from_csv(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open CSV file: " + path);
  }

  std::string header;
  if (!std::getline(file, header)) {
    throw std::runtime_error("CSV appears empty: " + path);
  }
  const std::vector<std::string> header_fields = split_csv_line(header);

  int date_index = -1;
  int close_index = -1;
  int adj_close_index = -1;
  for (size_t index = 0; index < header_fields.size(); ++index) {
    if (header_fields[index] == "Date") {
      date_index = static_cast<int>(index);
    } else if (header_fields[index] == "Close") {
      close_index = static_cast<int>(index);
    } else if (header_fields[index] == "Adj Close") {
      adj_close_index = static_cast<int>(index);
    }
  }

  if (date_index < 0 || (close_index < 0 && adj_close_index < 0)) {
    throw std::runtime_error("CSV requires at least Date and Close/Adj Close columns.");
  }

  // Ternary operator: condition ? value_if_true : value_if_false
  const int price_index = (adj_close_index >= 0) ? adj_close_index : close_index;

  std::vector<Candle> prices;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> fields = split_csv_line(line);
    if (static_cast<int>(fields.size()) <= std::max(date_index, price_index)) {
      continue;
    }
    if (fields[price_index] == "null" || fields[price_index].empty()) {
      continue;
    }

    Candle candle;
    candle.date = fields[date_index];
    candle.close = std::stod(fields[price_index]);
    prices.push_back(candle);
  }

  if (prices.size() < 10) {
    throw std::runtime_error("Not enough price rows for backtesting.");
  }
  return prices;
}

// Core backtest loop for one parameter set.
// `const std::vector<Candle>&` means "read-only reference" (avoid copying big arrays).
SimulationResult run_simulation(const std::vector<Candle>& prices, const StrategyParams& params, double initial_equity) {
  SimulationResult result;
  result.params = params;
  result.final_equity = initial_equity;

  if (prices.size() < static_cast<size_t>(params.lookback_days + 2)) {
    return result;
  }

  double cash = initial_equity;
  double shares = 0.0;
  double entry_value = 0.0;
  int days_left = 0;
  int wins = 0;
  int trades = 0;

  // Stores portfolio value each bar; used later for risk metrics.
  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());

  for (size_t index = 0; index < prices.size(); ++index) {
    const double close = prices[index].close;

    if (shares > 0.0) {
      days_left -= 1;
      if (days_left <= 0) {
        const double exit_value = shares * close;
        if (exit_value > entry_value) {
          wins += 1;
        }
        cash = exit_value;  // sell all shares back into cash
        shares = 0.0;
      }
    }

    const bool has_lookback = index >= static_cast<size_t>(params.lookback_days);
    if (shares == 0.0 && has_lookback) {
      const double lookback_close = prices[index - static_cast<size_t>(params.lookback_days)].close;
      if (lookback_close > 0.0) {
        const double drop = (close / lookback_close) - 1.0;
        if (drop <= -params.dip_pct) {
          shares = cash / close;
          entry_value = cash;
          cash = 0.0;  // fully invested (no leverage)
          days_left = params.hold_days;
          trades += 1;
        }
      }
    }

    equity_curve.push_back(cash + shares * close);
  }

  if (shares > 0.0) {
    const double final_value = shares * prices.back().close;
    if (final_value > entry_value) {
      wins += 1;
    }
    result.final_equity = final_value;
  } else {
    result.final_equity = cash;
  }

  result.metrics.total_return = (initial_equity > 0.0) ? ((result.final_equity / initial_equity) - 1.0) : 0.0;
  result.metrics.cagr = compute_cagr(initial_equity, result.final_equity, prices.size());
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve);
  result.metrics.trades = trades;
  result.metrics.win_rate = (trades > 0) ? (static_cast<double>(wins) / static_cast<double>(trades)) : 0.0;

  return result;
}

// Brute-force parameter sweep (nested loops like Python `for x ...: for y ...:`).
std::vector<SimulationResult> run_grid_search(const std::vector<Candle>& prices, const GridSearchConfig& config, double initial_equity) {
  std::vector<SimulationResult> results;
  for (double x = config.x_min; x <= config.x_max + 1e-9; x += config.x_step) {
    for (int y = config.y_min; y <= config.y_max; y += config.y_step) {
      StrategyParams params;
      params.dip_pct = x;
      params.lookback_days = y;
      params.hold_days = config.hold_days;

      results.push_back(run_simulation(prices, params, initial_equity));
    }
  }
  return results;
}
