#include "metis/backtest/simulation.hpp"

#include "metis/backtest/execution_accounting.hpp"
#include "metis/backtest/metrics.hpp"
#include "metis/strategy/strategy.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace metis {

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

}  // namespace metis
