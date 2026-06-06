#include "backtest.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
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
double compute_sharpe(const std::vector<double>& equity_curve, double bars_per_year) {
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

  return (mean / stddev) * std::sqrt(bars_per_year);
}

double compute_sortino(const std::vector<double>& equity_curve, double bars_per_year) {
  if (equity_curve.size() < 2) {
    return 0.0;
  }
  std::vector<double> returns;
  returns.reserve(equity_curve.size() - 1);
  for (size_t index = 1; index < equity_curve.size(); ++index) {
    const double prev = equity_curve[index - 1];
    const double current = equity_curve[index];
    returns.push_back(prev > 0.0 ? (current - prev) / prev : 0.0);
  }

  double mean = 0.0;
  for (double value : returns) {
    mean += value;
  }
  mean /= static_cast<double>(returns.size());

  double downside_variance = 0.0;
  size_t downside_count = 0;
  for (double value : returns) {
    if (value < 0.0) {
      downside_variance += value * value;
      downside_count += 1;
    }
  }
  if (downside_count == 0) {
    return 0.0;
  }

  const double downside_deviation = std::sqrt(downside_variance / static_cast<double>(downside_count));
  if (downside_deviation == 0.0) {
    return 0.0;
  }
  return (mean / downside_deviation) * std::sqrt(bars_per_year);
}

double compute_cagr(double initial_equity, double final_equity, size_t bars, double bars_per_year) {
  if (initial_equity <= 0.0 || final_equity <= 0.0 || bars < 2) {
    return 0.0;
  }
  const double years = static_cast<double>(bars - 1) / bars_per_year;
  if (years <= 0.0) {
    return 0.0;
  }
  return std::pow(final_equity / initial_equity, 1.0 / years) - 1.0;
}

double order_cost(double trade_value, const TransactionCosts& costs) {
  if (trade_value <= 0.0) {
    return 0.0;
  }
  return std::max(0.0, costs.fixed_per_order) + (std::max(0.0, costs.variable_rate) * trade_value);
}

double simple_moving_average(const std::vector<Candle>& prices, size_t end_index, int lookback) {
  if (lookback <= 0 || end_index + 1 < static_cast<size_t>(lookback)) {
    return 0.0;
  }
  double total = 0.0;
  const size_t start = end_index + 1 - static_cast<size_t>(lookback);
  for (size_t index = start; index <= end_index; ++index) {
    total += prices[index].close;
  }
  return total / static_cast<double>(lookback);
}

double realized_volatility(const std::vector<Candle>& prices, size_t end_index, int lookback, double bars_per_year) {
  if (lookback <= 1 || end_index < static_cast<size_t>(lookback)) {
    return 0.0;
  }
  std::vector<double> returns;
  returns.reserve(static_cast<size_t>(lookback));
  const size_t start = end_index + 1 - static_cast<size_t>(lookback);
  for (size_t index = start + 1; index <= end_index; ++index) {
    const double previous = prices[index - 1].close;
    returns.push_back(previous > 0.0 ? (prices[index].close / previous) - 1.0 : 0.0);
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
  return std::sqrt(variance) * std::sqrt(bars_per_year);
}

double position_fraction(
    const std::vector<Candle>& prices,
    size_t index,
    const StrategyParams& params,
    const Annualization& annualization) {
  const double max_position_pct = std::max(0.0, params.max_position_pct);
  if (params.target_volatility <= 0.0 || params.volatility_lookback_days <= 1) {
    return max_position_pct;
  }
  const double vol = realized_volatility(prices, index, params.volatility_lookback_days, annualization.bars_per_year);
  if (vol <= 0.0) {
    return 0.0;
  }
  return std::min(max_position_pct, params.target_volatility / vol);
}

int regime_state_direction(const std::vector<Candle>& prices, size_t index, const StrategyParams& params) {
  const double fast_now = simple_moving_average(prices, index, params.fast_lookback_days);
  const double slow_now = simple_moving_average(prices, index, params.lookback_days);
  if (fast_now > 0.0 && slow_now > 0.0 && fast_now > slow_now) {
    return 1;
  }
  if (params.allow_short) {
    const double short_fast_now = simple_moving_average(prices, index, params.short_fast_lookback_days);
    const double short_slow_now = simple_moving_average(prices, index, params.short_lookback_days);
    if (short_fast_now > 0.0 && short_slow_now > 0.0 && short_fast_now < short_slow_now) {
      return -1;
    }
  }
  return 0;
}

double median_double(std::vector<double> values, double fallback) {
  if (values.empty()) {
    return fallback;
  }
  std::sort(values.begin(), values.end());
  return values[values.size() / 2];
}

int median_int(std::vector<int> values, int fallback) {
  if (values.empty()) {
    return fallback;
  }
  std::sort(values.begin(), values.end());
  return values[values.size() / 2];
}

bool majority_bool(const std::vector<bool>& values, bool fallback) {
  if (values.empty()) {
    return fallback;
  }
  int true_count = 0;
  for (bool value : values) {
    if (value) {
      true_count += 1;
    }
  }
  return true_count * 2 >= static_cast<int>(values.size());
}

StrategyParams median_params_for_direction(const std::vector<StrategyParams>& params, const std::vector<int>& votes, int direction) {
  StrategyParams selected;
  selected.strategy = StrategyType::Regime;
  std::vector<double> take_profit_values;
  std::vector<double> stop_loss_values;
  std::vector<double> trailing_stop_values;
  std::vector<double> target_volatility_values;
  std::vector<double> max_position_values;
  std::vector<int> volatility_lookback_values;
  std::vector<bool> weak_exit_values;
  for (size_t index = 0; index < params.size(); ++index) {
    if (votes[index] != direction) {
      continue;
    }
    take_profit_values.push_back(params[index].take_profit_pct);
    stop_loss_values.push_back(params[index].stop_loss_pct);
    trailing_stop_values.push_back(params[index].trailing_stop_pct);
    volatility_lookback_values.push_back(params[index].volatility_lookback_days);
    target_volatility_values.push_back(params[index].target_volatility);
    max_position_values.push_back(params[index].max_position_pct);
    weak_exit_values.push_back(params[index].exit_on_regime_weakness);
  }
  selected.take_profit_pct = median_double(take_profit_values, 0.0);
  selected.stop_loss_pct = median_double(stop_loss_values, 0.0);
  selected.trailing_stop_pct = median_double(trailing_stop_values, 0.0);
  selected.volatility_lookback_days = median_int(volatility_lookback_values, 0);
  selected.target_volatility = median_double(target_volatility_values, 0.0);
  selected.max_position_pct = median_double(max_position_values, 1.0);
  selected.exit_on_regime_weakness = majority_bool(weak_exit_values, false);
  return selected;
}
}  // namespace

StrategyType strategy_type_from_string(const std::string& value) {
  if (value == "drop" || value == "dip") {
    return StrategyType::Drop;
  }
  if (value == "gain" || value == "momentum") {
    return StrategyType::Gain;
  }
  if (value == "regime" || value == "trend") {
    return StrategyType::Regime;
  }
  throw std::runtime_error("Unknown strategy type: " + value + ". Use 'drop', 'gain', or 'regime'.");
}

std::string strategy_type_to_string(StrategyType strategy) {
  if (strategy == StrategyType::Gain) {
    return "gain";
  }
  if (strategy == StrategyType::Regime) {
    return "regime";
  }
  return "drop";
}

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
SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const StrategyParams& params,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  SimulationResult result;
  result.params = params;
  result.final_equity = initial_equity;

  int required_lookback =
      params.strategy == StrategyType::Regime ? std::max(params.lookback_days, params.fast_lookback_days) : params.lookback_days;
  if (params.strategy == StrategyType::Regime && params.allow_short) {
    required_lookback = std::max(required_lookback, std::max(params.short_lookback_days, params.short_fast_lookback_days));
  }
  if (params.volatility_lookback_days > 0) {
    required_lookback = std::max(required_lookback, params.volatility_lookback_days);
  }
  if (prices.size() < static_cast<size_t>(required_lookback + 2)) {
    return result;
  }

  double cash = initial_equity;
  double shares = 0.0;
  double entry_value = 0.0;
  double entry_price = 0.0;
  double entry_cost = 0.0;
  double entry_signal_quality = 0.0;
  double highest_price_since_entry = 0.0;
  double lowest_price_since_entry = 0.0;
  std::string entry_date;
  int position_direction = 0;
  int days_left = 0;
  int held_days = 0;
  int wins = 0;
  int trades = 0;
  bool trailing_stop_active = false;

  // Stores portfolio value each bar; used later for risk metrics.
  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());
  result.equity_curve.reserve(prices.size());

  for (size_t index = 0; index < prices.size(); ++index) {
    const double close = prices[index].close;

    if (shares > 0.0) {
      days_left -= 1;
      held_days += 1;
      if (close > highest_price_since_entry) {
        highest_price_since_entry = close;
      }
      if (lowest_price_since_entry <= 0.0 || close < lowest_price_since_entry) {
        lowest_price_since_entry = close;
      }
      const double trade_return =
          entry_price > 0.0 ? (static_cast<double>(position_direction) * ((close / entry_price) - 1.0)) : 0.0;
      if (params.take_profit_pct > 0.0 && trade_return >= params.take_profit_pct) {
        trailing_stop_active = true;
      }
      const bool take_profit_exit =
          params.take_profit_pct > 0.0 && trade_return >= params.take_profit_pct && params.trailing_stop_pct <= 0.0;
      const bool trailing_stop_exit =
          trailing_stop_active && params.trailing_stop_pct > 0.0 &&
          ((position_direction > 0 && highest_price_since_entry > 0.0 &&
            close <= highest_price_since_entry * (1.0 - params.trailing_stop_pct)) ||
           (position_direction < 0 && lowest_price_since_entry > 0.0 &&
            close >= lowest_price_since_entry * (1.0 + params.trailing_stop_pct)));
      const bool stop_loss_exit = params.stop_loss_pct > 0.0 && trade_return <= -params.stop_loss_pct;
      const bool timed_exit = params.strategy != StrategyType::Regime && days_left <= 0;
      bool regime_weakness_exit = false;
      if (params.strategy == StrategyType::Regime && params.exit_on_regime_weakness) {
        if (position_direction > 0) {
          const double fast_now = simple_moving_average(prices, index, params.fast_lookback_days);
          const double slow_now = simple_moving_average(prices, index, params.lookback_days);
          regime_weakness_exit = fast_now > 0.0 && slow_now > 0.0 && fast_now < slow_now;
        } else if (position_direction < 0) {
          const double fast_now = simple_moving_average(prices, index, params.short_fast_lookback_days);
          const double slow_now = simple_moving_average(prices, index, params.short_lookback_days);
          regime_weakness_exit = fast_now > 0.0 && slow_now > 0.0 && fast_now > slow_now;
        }
      }
      if (timed_exit || take_profit_exit || trailing_stop_exit || stop_loss_exit || regime_weakness_exit) {
        const double exit_value = shares * close;
        const double exit_cost = order_cost(exit_value, costs);
        if (position_direction > 0) {
          cash += std::max(0.0, exit_value - exit_cost);
        } else {
          cash -= exit_value + exit_cost;
        }
        const double pnl = cash - entry_value;
        if (pnl > 0.0) {
          wins += 1;
        }
        Trade trade;
        trade.entry_date = entry_date;
        trade.exit_date = prices[index].date;
        trade.direction = position_direction > 0 ? "long" : "short";
        trade.entry_price = entry_price;
        trade.exit_price = close;
        trade.shares = shares;
        trade.pnl = pnl;
        trade.costs = entry_cost + exit_cost;
        trade.entry_signal_quality = entry_signal_quality;
        trade.holding_days = held_days;
        result.trades.push_back(trade);
        shares = 0.0;
        position_direction = 0;
      }
    }

    const bool has_lookback = index >= static_cast<size_t>(required_lookback);
    if (shares == 0.0 && has_lookback) {
      bool should_enter = false;
      int entry_direction = 0;
      if (params.strategy == StrategyType::Regime) {
        const double fast_now = simple_moving_average(prices, index, params.fast_lookback_days);
        const double slow_now = simple_moving_average(prices, index, params.lookback_days);
        const double fast_prev = simple_moving_average(prices, index - 1, params.fast_lookback_days);
        const double slow_prev = simple_moving_average(prices, index - 1, params.lookback_days);
        should_enter = fast_prev > 0.0 && slow_prev > 0.0 && fast_now > 0.0 && slow_now > 0.0 &&
                       fast_prev <= slow_prev && fast_now > slow_now;
        if (should_enter) {
          entry_signal_quality = (fast_now - slow_now) / slow_now;
          entry_direction = 1;
        } else if (params.allow_short) {
          const double short_fast_now = simple_moving_average(prices, index, params.short_fast_lookback_days);
          const double short_slow_now = simple_moving_average(prices, index, params.short_lookback_days);
          const double short_fast_prev = simple_moving_average(prices, index - 1, params.short_fast_lookback_days);
          const double short_slow_prev = simple_moving_average(prices, index - 1, params.short_lookback_days);
          should_enter = short_fast_prev > 0.0 && short_slow_prev > 0.0 && short_fast_now > 0.0 &&
                         short_slow_now > 0.0 && short_fast_prev >= short_slow_prev &&
                         short_fast_now < short_slow_now;
          if (should_enter) {
            entry_signal_quality = (short_slow_now - short_fast_now) / short_slow_now;
            entry_direction = -1;
          }
        }
      } else {
        const double lookback_close = prices[index - static_cast<size_t>(params.lookback_days)].close;
        if (lookback_close > 0.0) {
          const double diff = (close / lookback_close) - 1.0;
          const double threshold = std::abs(params.diff_pct);
          should_enter = params.strategy == StrategyType::Drop ? diff <= -threshold : diff >= threshold;
          if (should_enter) {
            entry_signal_quality = diff;
            entry_direction = 1;
          }
        }
      }
      if (should_enter) {
          const double equity = cash;
          const double fraction = position_fraction(prices, index, params, annualization);
          const double target_notional = equity * std::max(0.0, fraction);
          const double entry_cost_budget = std::min(cash, target_notional) - costs.fixed_per_order;
          const double denominator = close * (1.0 + std::max(0.0, costs.variable_rate));
          if (entry_cost_budget <= 0.0 || denominator <= 0.0) {
            continue;
          }
          shares = entry_cost_budget / denominator;
          const double entry_trade_value = shares * close;
          entry_value = equity;
          entry_price = close;
          entry_cost = order_cost(entry_trade_value, costs);
          highest_price_since_entry = close;
          lowest_price_since_entry = close;
          entry_date = prices[index].date;
          position_direction = entry_direction;
          if (position_direction > 0) {
            cash -= entry_trade_value + entry_cost;
          } else {
            cash += entry_trade_value - entry_cost;
          }
          days_left = params.hold_days;
          held_days = 0;
          trailing_stop_active = false;
          trades += 1;
      }
    }

    const double equity = cash + static_cast<double>(position_direction) * shares * close;
    equity_curve.push_back(equity);
    result.equity_curve.push_back({prices[index].date, equity});
  }

  if (shares > 0.0) {
    const double final_value = shares * prices.back().close;
    const double final_cost = order_cost(final_value, costs);
    if (position_direction > 0) {
      cash += std::max(0.0, final_value - final_cost);
    } else {
      cash -= final_value + final_cost;
    }
    const double pnl = cash - entry_value;
    if (pnl > 0.0) {
      wins += 1;
    }
    Trade trade;
    trade.entry_date = entry_date;
    trade.exit_date = prices.back().date;
    trade.direction = position_direction > 0 ? "long" : "short";
    trade.entry_price = entry_price;
    trade.exit_price = prices.back().close;
    trade.shares = shares;
    trade.pnl = pnl;
    trade.costs = entry_cost + final_cost;
    trade.entry_signal_quality = entry_signal_quality;
    trade.holding_days = held_days;
    result.trades.push_back(trade);
    result.final_equity = cash;
  } else {
    result.final_equity = cash;
  }

  result.metrics.total_return = (initial_equity > 0.0) ? ((result.final_equity / initial_equity) - 1.0) : 0.0;
  result.metrics.cagr = compute_cagr(initial_equity, result.final_equity, prices.size(), annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = trades;
  result.metrics.win_rate = (trades > 0) ? (static_cast<double>(wins) / static_cast<double>(trades)) : 0.0;

  return result;
}

SimulationResult run_buy_and_hold(
    const std::vector<Candle>& prices,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  SimulationResult result;
  result.final_equity = initial_equity;
  if (prices.size() < 2 || initial_equity <= 0.0 || prices.front().close <= 0.0) {
    return result;
  }

  const double entry_price = prices.front().close;
  const double entry_cost_budget = initial_equity - costs.fixed_per_order;
  const double denominator = entry_price * (1.0 + std::max(0.0, costs.variable_rate)); //Adjust the costs, make them more like IBKR
  if (entry_cost_budget <= 0.0 || denominator <= 0.0) {
    return result;
  }

  const double shares = entry_cost_budget / denominator;
  const double entry_trade_value = shares * entry_price;
  const double entry_cost = order_cost(entry_trade_value, costs);

  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());
  result.equity_curve.reserve(prices.size());
  for (const Candle& candle : prices) {
    const double equity = shares * candle.close;
    equity_curve.push_back(equity);
    result.equity_curve.push_back({candle.date, equity});
  }

  const double exit_trade_value = shares * prices.back().close;
  const double exit_cost = order_cost(exit_trade_value, costs);
  const double net_exit_value = std::max(0.0, exit_trade_value - exit_cost);

  Trade trade;
  trade.entry_date = prices.front().date;
  trade.exit_date = prices.back().date;
  trade.entry_price = entry_price;
  trade.exit_price = prices.back().close;
  trade.shares = shares;
  trade.pnl = net_exit_value - initial_equity;
  trade.costs = entry_cost + exit_cost;
  trade.holding_days = static_cast<int>(prices.size() - 1); // This is wrong if the bars are hours
  result.trades.push_back(trade);

  if (!result.equity_curve.empty()) {
    result.equity_curve.back().equity = net_exit_value;
    equity_curve.back() = net_exit_value;
  }

  result.final_equity = net_exit_value;
  result.metrics.total_return = (result.final_equity / initial_equity) - 1.0;
  result.metrics.cagr = compute_cagr(initial_equity, result.final_equity, prices.size(), annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = 1;
  result.metrics.win_rate = trade.pnl > 0.0 ? 1.0 : 0.0;
  return result;
}

SimulationResult run_ensemble_simulation(
    const std::vector<Candle>& prices,
    const std::vector<StrategyParams>& params,
    bool rank_weighted,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  SimulationResult result;
  result.final_equity = initial_equity;
  if (prices.size() < 10 || params.empty()) {
    return result;
  }
  result.params = params.front();

  int required_lookback = 1;
  for (const StrategyParams& item : params) {
    required_lookback = std::max(required_lookback, item.lookback_days);
    required_lookback = std::max(required_lookback, item.fast_lookback_days);
    if (item.allow_short) {
      required_lookback = std::max(required_lookback, item.short_lookback_days);
      required_lookback = std::max(required_lookback, item.short_fast_lookback_days);
    }
    required_lookback = std::max(required_lookback, item.volatility_lookback_days);
  }
  if (prices.size() < static_cast<size_t>(required_lookback + 2)) {
    return result;
  }

  double cash = initial_equity;
  double shares = 0.0;
  double entry_value = 0.0;
  double entry_price = 0.0;
  double entry_cost = 0.0;
  double highest_price_since_entry = 0.0;
  double lowest_price_since_entry = 0.0;
  double entry_signal_quality = 0.0;
  std::string entry_date;
  int position_direction = 0;
  int held_days = 0;
  int wins = 0;
  int trades = 0;
  bool trailing_stop_active = false;
  StrategyParams active_risk_params;

  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());
  result.equity_curve.reserve(prices.size());

  const int majority_threshold = static_cast<int>(params.size() / 2) + 1;
  const double total_weight = (static_cast<double>(params.size()) * (static_cast<double>(params.size()) + 1.0)) / 2.0;
  const double weighted_threshold = total_weight / 2.0;
  for (size_t index = 0; index < prices.size(); ++index) {
    const double close = prices[index].close;
    std::vector<int> votes(params.size(), 0);
    int long_votes = 0;
    int short_votes = 0;
    double long_weight = 0.0;
    double short_weight = 0.0;
    if (index >= static_cast<size_t>(required_lookback)) {
      for (size_t param_index = 0; param_index < params.size(); ++param_index) {
        votes[param_index] = regime_state_direction(prices, index, params[param_index]);
        const double rank_weight = static_cast<double>(params.size() - param_index);
        if (votes[param_index] > 0) {
          long_votes += 1;
          long_weight += rank_weight;
        } else if (votes[param_index] < 0) {
          short_votes += 1;
          short_weight += rank_weight;
        }
      }
    }
    int ensemble_direction = 0;
    if (rank_weighted) {
      if (long_weight > weighted_threshold && long_weight > short_weight) {
        ensemble_direction = 1;
      } else if (short_weight > weighted_threshold && short_weight > long_weight) {
        ensemble_direction = -1;
      }
    } else {
      if (long_votes >= majority_threshold && long_votes > short_votes) {
        ensemble_direction = 1;
      } else if (short_votes >= majority_threshold && short_votes > long_votes) {
        ensemble_direction = -1;
      }
    }

    if (shares > 0.0) {
      held_days += 1;
      highest_price_since_entry = std::max(highest_price_since_entry, close);
      lowest_price_since_entry = lowest_price_since_entry <= 0.0 ? close : std::min(lowest_price_since_entry, close);
      const double trade_return =
          entry_price > 0.0 ? (static_cast<double>(position_direction) * ((close / entry_price) - 1.0)) : 0.0;
      if (active_risk_params.take_profit_pct > 0.0 && trade_return >= active_risk_params.take_profit_pct) {
        trailing_stop_active = true;
      }
      const bool take_profit_exit = active_risk_params.take_profit_pct > 0.0 &&
                                    trade_return >= active_risk_params.take_profit_pct &&
                                    active_risk_params.trailing_stop_pct <= 0.0;
      const bool trailing_stop_exit =
          trailing_stop_active && active_risk_params.trailing_stop_pct > 0.0 &&
          ((position_direction > 0 && close <= highest_price_since_entry * (1.0 - active_risk_params.trailing_stop_pct)) ||
           (position_direction < 0 && close >= lowest_price_since_entry * (1.0 + active_risk_params.trailing_stop_pct)));
      const bool stop_loss_exit = active_risk_params.stop_loss_pct > 0.0 && trade_return <= -active_risk_params.stop_loss_pct;
      const bool consensus_lost_exit = ensemble_direction != position_direction;
      if (take_profit_exit || trailing_stop_exit || stop_loss_exit || consensus_lost_exit) {
        const double exit_value = shares * close;
        const double exit_cost = order_cost(exit_value, costs);
        if (position_direction > 0) {
          cash += std::max(0.0, exit_value - exit_cost);
        } else {
          cash -= exit_value + exit_cost;
        }
        const double pnl = cash - entry_value;
        if (pnl > 0.0) {
          wins += 1;
        }
        Trade trade;
        trade.entry_date = entry_date;
        trade.exit_date = prices[index].date;
        trade.direction = position_direction > 0 ? "long" : "short";
        trade.entry_price = entry_price;
        trade.exit_price = close;
        trade.shares = shares;
        trade.pnl = pnl;
        trade.costs = entry_cost + exit_cost;
        trade.entry_signal_quality = entry_signal_quality;
        trade.holding_days = held_days;
        result.trades.push_back(trade);
        shares = 0.0;
        position_direction = 0;
      }
    }

    if (shares == 0.0 && ensemble_direction != 0 && index >= static_cast<size_t>(required_lookback)) {
      active_risk_params = median_params_for_direction(params, votes, ensemble_direction);
      const double equity = cash;
      const double fraction = position_fraction(prices, index, active_risk_params, annualization);
      const double target_notional = equity * std::max(0.0, fraction);
      const double entry_cost_budget = std::min(cash, target_notional) - costs.fixed_per_order;
      const double denominator = close * (1.0 + std::max(0.0, costs.variable_rate));
      if (entry_cost_budget > 0.0 && denominator > 0.0) {
        shares = entry_cost_budget / denominator;
        const double entry_trade_value = shares * close;
        entry_value = equity;
        entry_price = close;
        entry_cost = order_cost(entry_trade_value, costs);
        highest_price_since_entry = close;
        lowest_price_since_entry = close;
        entry_date = prices[index].date;
        position_direction = ensemble_direction;
        entry_signal_quality = rank_weighted
                                   ? (position_direction > 0 ? long_weight : short_weight) / total_weight
                                   : static_cast<double>(position_direction > 0 ? long_votes : short_votes) /
                                         static_cast<double>(params.size());
        if (position_direction > 0) {
          cash -= entry_trade_value + entry_cost;
        } else {
          cash += entry_trade_value - entry_cost;
        }
        held_days = 0;
        trailing_stop_active = false;
        trades += 1;
      }
    }

    const double equity = cash + static_cast<double>(position_direction) * shares * close;
    equity_curve.push_back(equity);
    result.equity_curve.push_back({prices[index].date, equity});
  }

  if (shares > 0.0) {
    const double final_value = shares * prices.back().close;
    const double final_cost = order_cost(final_value, costs);
    if (position_direction > 0) {
      cash += std::max(0.0, final_value - final_cost);
    } else {
      cash -= final_value + final_cost;
    }
    const double pnl = cash - entry_value;
    if (pnl > 0.0) {
      wins += 1;
    }
    Trade trade;
    trade.entry_date = entry_date;
    trade.exit_date = prices.back().date;
    trade.direction = position_direction > 0 ? "long" : "short";
    trade.entry_price = entry_price;
    trade.exit_price = prices.back().close;
    trade.shares = shares;
    trade.pnl = pnl;
    trade.costs = entry_cost + final_cost;
    trade.entry_signal_quality = entry_signal_quality;
    trade.holding_days = held_days;
    result.trades.push_back(trade);
  }

  result.final_equity = cash;
  result.metrics.total_return = (initial_equity > 0.0) ? ((result.final_equity / initial_equity) - 1.0) : 0.0;
  result.metrics.cagr = compute_cagr(initial_equity, result.final_equity, prices.size(), annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = trades;
  result.metrics.win_rate = (trades > 0) ? (static_cast<double>(wins) / static_cast<double>(trades)) : 0.0;
  return result;
}

// Brute-force parameter sweep (nested loops like Python `for x ...: for y ...:`).
std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  std::vector<SimulationResult> results;
  std::mt19937 random_engine(config.grid_sample_seed);
  std::uniform_real_distribution<double> sample_distribution(0.0, 1.0);
  auto should_evaluate_candidate = [&]() {
    return config.grid_sample_pct >= 1.0 || sample_distribution(random_engine) <= config.grid_sample_pct;
  };
  if (strategy == StrategyType::Regime) {
    if (config.grid_random_samples > 0) {
      auto int_range = [](int min_value, int max_value, int step) {
        std::vector<int> values;
        for (int value = min_value; value <= max_value; value += step) {
          values.push_back(value);
        }
        return values;
      };
      auto double_range = [](double min_value, double max_value, double step) {
        std::vector<double> values;
        for (double value = min_value; value <= max_value + 1e-9; value += step) {
          values.push_back(value);
        }
        return values;
      };
      auto pick_int = [&](const std::vector<int>& values) {
        std::uniform_int_distribution<size_t> distribution(0, values.size() - 1);
        return values[distribution(random_engine)];
      };
      auto pick_double = [&](const std::vector<double>& values) {
        std::uniform_int_distribution<size_t> distribution(0, values.size() - 1);
        return values[distribution(random_engine)];
      };

      const std::vector<int> slow_values = int_range(config.y_min, config.y_max, config.y_step);
      const std::vector<int> fast_values = int_range(config.fast_lookback_min, config.fast_lookback_max, config.fast_lookback_step);
      const std::vector<int> allow_short_values = int_range(config.allow_short_min, config.allow_short_max, 1);
      const std::vector<int> short_slow_values =
          config.short_y_min > 0 ? int_range(config.short_y_min, config.short_y_max, config.short_y_step) : std::vector<int>{0};
      const std::vector<int> short_fast_values =
          config.short_fast_lookback_min > 0
              ? int_range(config.short_fast_lookback_min, config.short_fast_lookback_max, config.short_fast_lookback_step)
              : std::vector<int>{0};
      const std::vector<int> vol_lookback_values =
          int_range(config.volatility_lookback_min, config.volatility_lookback_max, config.volatility_lookback_step);
      const std::vector<double> target_volatility_values =
          double_range(config.target_volatility_min, config.target_volatility_max, config.target_volatility_step);
      const std::vector<double> max_position_values =
          double_range(config.max_position_pct_min, config.max_position_pct_max, config.max_position_pct_step);
      const std::vector<double> take_profit_values = double_range(config.take_profit_min, config.take_profit_max, config.take_profit_step);
      const std::vector<double> stop_loss_values = double_range(config.stop_loss_min, config.stop_loss_max, config.stop_loss_step);
      const std::vector<double> trailing_stop_values =
          double_range(config.trailing_stop_min, config.trailing_stop_max, config.trailing_stop_step);
      const std::vector<int> weak_exit_values = int_range(config.regime_weak_exit_min, config.regime_weak_exit_max, 1);

      int attempts = 0;
      const int max_attempts = config.grid_random_samples * 50;
      while (static_cast<int>(results.size()) < config.grid_random_samples && attempts < max_attempts) {
        attempts += 1;
        const int y = pick_int(slow_values);
        const int fast_lookback = pick_int(fast_values);
        if (fast_lookback >= y) {
          continue;
        }
        const int allow_short = pick_int(allow_short_values);
        const int short_y = allow_short != 0 && config.short_y_min > 0 ? pick_int(short_slow_values) : y;
        const int short_fast_lookback =
            allow_short != 0 && config.short_fast_lookback_min > 0 ? pick_int(short_fast_values) : fast_lookback;
        if (short_fast_lookback >= short_y) {
          continue;
        }

        StrategyParams params;
        params.strategy = strategy;
        params.lookback_days = y;
        params.fast_lookback_days = fast_lookback;
        params.short_lookback_days = short_y;
        params.short_fast_lookback_days = short_fast_lookback;
        params.take_profit_pct = pick_double(take_profit_values);
        params.stop_loss_pct = pick_double(stop_loss_values);
        params.trailing_stop_pct = pick_double(trailing_stop_values);
        params.exit_on_regime_weakness = pick_int(weak_exit_values) != 0;
        params.allow_short = allow_short != 0;
        params.volatility_lookback_days = pick_int(vol_lookback_values);
        params.target_volatility = pick_double(target_volatility_values);
        params.max_position_pct = pick_double(max_position_values);

        results.push_back(run_simulation(prices, params, initial_equity, costs, annualization));
      }
      return results;
    }

    for (int y = config.y_min; y <= config.y_max; y += config.y_step) {
      for (int fast_lookback = config.fast_lookback_min; fast_lookback <= config.fast_lookback_max; fast_lookback += config.fast_lookback_step) {
        if (fast_lookback >= y) {
          continue;
        }
        for (int allow_short = config.allow_short_min; allow_short <= config.allow_short_max; ++allow_short) {
          const int short_y_min = allow_short != 0 && config.short_y_min > 0 ? config.short_y_min : y;
          const int short_y_max = allow_short != 0 && config.short_y_max > 0 ? config.short_y_max : y;
          const int short_y_step = allow_short != 0 && config.short_y_min > 0 ? config.short_y_step : 1;
          const int short_fast_min = allow_short != 0 && config.short_fast_lookback_min > 0 ? config.short_fast_lookback_min : fast_lookback;
          const int short_fast_max = allow_short != 0 && config.short_fast_lookback_max > 0 ? config.short_fast_lookback_max : fast_lookback;
          const int short_fast_step = allow_short != 0 && config.short_fast_lookback_min > 0 ? config.short_fast_lookback_step : 1;
          for (int short_y = short_y_min; short_y <= short_y_max; short_y += short_y_step) {
            for (int short_fast_lookback = short_fast_min; short_fast_lookback <= short_fast_max; short_fast_lookback += short_fast_step) {
              if (short_fast_lookback >= short_y) {
                continue;
              }
              for (int vol_lookback = config.volatility_lookback_min; vol_lookback <= config.volatility_lookback_max; vol_lookback += config.volatility_lookback_step) {
                for (double target_volatility = config.target_volatility_min; target_volatility <= config.target_volatility_max + 1e-9; target_volatility += config.target_volatility_step) {
                  for (double max_position_pct = config.max_position_pct_min; max_position_pct <= config.max_position_pct_max + 1e-9; max_position_pct += config.max_position_pct_step) {
                    for (double take_profit = config.take_profit_min; take_profit <= config.take_profit_max + 1e-9; take_profit += config.take_profit_step) {
                      for (double stop_loss = config.stop_loss_min; stop_loss <= config.stop_loss_max + 1e-9; stop_loss += config.stop_loss_step) {
                        for (double trailing_stop = config.trailing_stop_min; trailing_stop <= config.trailing_stop_max + 1e-9; trailing_stop += config.trailing_stop_step) {
                          for (int regime_weak_exit = config.regime_weak_exit_min; regime_weak_exit <= config.regime_weak_exit_max; ++regime_weak_exit) {
                            StrategyParams params;
                            params.strategy = strategy;
                            params.lookback_days = y;
                            params.fast_lookback_days = fast_lookback;
                            params.short_lookback_days = short_y;
                            params.short_fast_lookback_days = short_fast_lookback;
                            params.take_profit_pct = take_profit;
                            params.stop_loss_pct = stop_loss;
                            params.trailing_stop_pct = trailing_stop;
                            params.exit_on_regime_weakness = regime_weak_exit != 0;
                            params.allow_short = allow_short != 0;
                            params.volatility_lookback_days = vol_lookback;
                            params.target_volatility = target_volatility;
                            params.max_position_pct = max_position_pct;

                            if (should_evaluate_candidate()) {
                              results.push_back(run_simulation(prices, params, initial_equity, costs, annualization));
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return results;
  }

  for (double x = config.x_min; x <= config.x_max + 1e-9; x += config.x_step) {
    for (int y = config.y_min; y <= config.y_max; y += config.y_step) {
      for (int hold_days = config.hold_days_min; hold_days <= config.hold_days_max; hold_days += config.hold_days_step) {
        for (double take_profit = config.take_profit_min; take_profit <= config.take_profit_max + 1e-9; take_profit += config.take_profit_step) {
          for (double stop_loss = config.stop_loss_min; stop_loss <= config.stop_loss_max + 1e-9; stop_loss += config.stop_loss_step) {
            for (double trailing_stop = config.trailing_stop_min; trailing_stop <= config.trailing_stop_max + 1e-9; trailing_stop += config.trailing_stop_step) {
              StrategyParams params;
              params.strategy = strategy;
              params.diff_pct = x;
              params.lookback_days = y;
              params.hold_days = hold_days;
              params.take_profit_pct = take_profit;
              params.stop_loss_pct = stop_loss;
              params.trailing_stop_pct = trailing_stop;

              if (should_evaluate_candidate()) {
                results.push_back(run_simulation(prices, params, initial_equity, costs, annualization));
              }
            }
          }
        }
      }
    }
  }
  return results;
}

void sort_results_by_cagr(std::vector<SimulationResult>& results) {
  std::sort(results.begin(), results.end(), [](const SimulationResult& left, const SimulationResult& right) {
    if (left.metrics.cagr == right.metrics.cagr) {
      return left.final_equity > right.final_equity;
    }
    return left.metrics.cagr > right.metrics.cagr;
  });
}
