#include "metis/analytics/indicators.hpp"
#include "metis/backtest/metrics.hpp"
#include "metis/backtest/simulation.hpp"
#include "metis/optimization/discrete_grid_search.hpp"
#include "metis/strategy/strategy.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <string>

namespace metis {

// Anonymous namespace = file-private helpers (similar to private static helpers in a Java class).
namespace {
double order_cost(double trade_value, const TransactionCosts& costs) {
  if (trade_value <= 0.0) {
    return 0.0;
  }
  return std::max(0.0, costs.fixed_per_order) + (std::max(0.0, costs.variable_rate) * trade_value);
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

}  // namespace

// Core backtest loop for one parameter set.
// `const std::vector<Candle>&` means "read-only reference" (avoid copying big arrays).
SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const Strategy& strategy,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  SimulationResult result;
  const StrategyParams& params = strategy.params();
  result.params = params;
  result.final_equity = initial_equity;

  int required_lookback = strategy.required_lookback();
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
      const bool timed_exit = strategy.uses_timed_exit() && days_left <= 0;
      const bool regime_weakness_exit = strategy.should_exit_on_signal_weakness(prices, index, position_direction);
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
      int entry_direction = 0;
      const PortfolioState state{cash, position_direction};
      const Signal signal = strategy.signal(prices, index, state);
      const bool should_enter = signal.direction == Direction::Long || signal.direction == Direction::Short;
      if (signal.direction == Direction::Long) {
        entry_direction = 1;
      } else if (signal.direction == Direction::Short) {
        entry_direction = -1;
      }
      entry_signal_quality = signal.confidence;
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

  result.metrics.total_return = compute_total_return(initial_equity, result.final_equity);
  result.metrics.cagr = compute_cagr_from_bars(initial_equity, result.final_equity, prices.size(), annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = trades;
  result.metrics.win_rate = (trades > 0) ? (static_cast<double>(wins) / static_cast<double>(trades)) : 0.0;

  return result;
}

SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const Strategy& strategy,
    const ExecutionConfig& execution) {
  return run_simulation(prices, strategy, execution.initial_equity, execution.costs, execution.annualization);
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
  result.metrics.total_return = compute_total_return(initial_equity, result.final_equity);
  result.metrics.cagr = compute_cagr_from_bars(initial_equity, result.final_equity, prices.size(), annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = 1;
  result.metrics.win_rate = trade.pnl > 0.0 ? 1.0 : 0.0;
  return result;
}

// Brute-force parameter sweep (nested loops like Python `for x ...: for y ...:`).
DiscreteGridSearchOptimizer::DiscreteGridSearchOptimizer(DiscreteStrategySearchConfig config) : config_(config) {}

std::vector<SimulationResult> DiscreteGridSearchOptimizer::evaluate(
    const std::vector<Candle>& prices,
    const ExecutionConfig& execution) const {
  return run_grid_search(prices, config_.grid, config_.strategy, execution);
}

std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    const ExecutionConfig& execution) {
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

        const DiscreteStrategy candidate(params);
        results.push_back(run_simulation(prices, candidate, execution));
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
                              const DiscreteStrategy candidate(params);
                              results.push_back(run_simulation(prices, candidate, execution));
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
                const DiscreteStrategy candidate(params);
                results.push_back(run_simulation(prices, candidate, execution));
              }
            }
          }
        }
      }
    }
  }
  return results;
}

}  // namespace metis
