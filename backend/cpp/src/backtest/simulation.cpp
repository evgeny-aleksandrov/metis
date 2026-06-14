#include "metis/backtest/simulation.hpp"

#include "metis/backtest/execution_accounting.hpp"
#include "metis/backtest/metrics.hpp"
#include "metis/backtest/trade_exit_policy.hpp"
#include "metis/strategy/strategy.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

namespace metis {

SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const Strategy& strategy,
    const SimulationRules& rules,
    double initial_equity,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  SimulationResult result;
  const StrategyParams& params = strategy.params();
  result.params = params;
  result.final_equity = initial_equity;

  int required_lookback = strategy.required_lookback();
  if (rules.sizing.volatility_lookback_days > 0) {
    required_lookback = std::max(required_lookback, rules.sizing.volatility_lookback_days);
  }
  if (prices.size() < static_cast<size_t>(required_lookback + 2)) {
    return result;
  }

  double cash = initial_equity;
  double entry_cost = 0.0;
  int wins = 0;
  int trades = 0;
  OpenPosition position;
  std::unique_ptr<TradeExitPolicy> exit_policy =
      create_trade_exit_policy(rules.trade_management, strategy);

  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());
  result.equity_curve.reserve(prices.size());

  for (size_t index = 0; index < prices.size(); ++index) {
    const double close = prices[index].close;

    if (position.is_open()) {
      position.held_days += 1;
      const ExitDecision exit = exit_policy->evaluate(prices, index, position);
      if (exit.should_exit) {
        const double exit_value = position.shares * close;
        const double exit_cost = order_cost(exit_value, costs);
        if (position.direction > 0) {
          cash += std::max(0.0, exit_value - exit_cost);
        } else {
          cash -= exit_value + exit_cost;
        }
        const double pnl = cash - position.entry_value;
        if (pnl > 0.0) {
          wins += 1;
        }
        Trade trade;
        trade.entry_date = position.entry_date;
        trade.exit_date = prices[index].date;
        trade.direction = position.direction > 0 ? "long" : "short";
        trade.entry_price = position.entry_price;
        trade.exit_price = close;
        trade.shares = position.shares;
        trade.pnl = pnl;
        trade.costs = entry_cost + exit_cost;
        trade.entry_signal_quality = position.entry_signal_quality;
        trade.holding_days = position.held_days;
        result.trades.push_back(trade);
        position = {};
      }
    }

    const bool has_lookback = index >= static_cast<size_t>(required_lookback);
    if (!position.is_open() && has_lookback) {
      int entry_direction = 0;
      const PortfolioState state{cash, position.direction};
      const Signal signal = strategy.signal(prices, index, state);
      const bool should_enter = signal.direction == Direction::Long || signal.direction == Direction::Short;
      if (signal.direction == Direction::Long) {
        entry_direction = 1;
      } else if (signal.direction == Direction::Short) {
        entry_direction = -1;
      }
      if (should_enter) {
        const double equity = cash;
        const double fraction = position_fraction(
            prices,
            index,
            rules.sizing.max_position_pct,
            rules.sizing.target_volatility,
            rules.sizing.volatility_lookback_days,
            annualization);
        const double target_notional = equity * std::max(0.0, fraction);
        const double entry_cost_budget = std::min(cash, target_notional) - costs.fixed_per_order;
        const double denominator = close * (1.0 + std::max(0.0, costs.variable_rate));
        if (entry_cost_budget <= 0.0 || denominator <= 0.0) {
          continue;
        }
        position.shares = entry_cost_budget / denominator;
        const double entry_trade_value = position.shares * close;
        position.entry_value = equity;
        position.entry_price = close;
        entry_cost = order_cost(entry_trade_value, costs);
        position.entry_date = prices[index].date;
        position.direction = entry_direction;
        position.entry_signal_quality = signal.confidence;
        if (position.direction > 0) {
          cash -= entry_trade_value + entry_cost;
        } else {
          cash += entry_trade_value - entry_cost;
        }
        position.held_days = 0;
        exit_policy->on_entry(position);
        trades += 1;
      }
    }

    const double equity = cash + static_cast<double>(position.direction) * position.shares * close;
    equity_curve.push_back(equity);
    result.equity_curve.push_back({prices[index].date, equity});
  }

  if (position.is_open()) {
    const double final_value = position.shares * prices.back().close;
    const double final_cost = order_cost(final_value, costs);
    if (position.direction > 0) {
      cash += std::max(0.0, final_value - final_cost);
    } else {
      cash -= final_value + final_cost;
    }
    const double pnl = cash - position.entry_value;
    if (pnl > 0.0) {
      wins += 1;
    }
    Trade trade;
    trade.entry_date = position.entry_date;
    trade.exit_date = prices.back().date;
    trade.direction = position.direction > 0 ? "long" : "short";
    trade.entry_price = position.entry_price;
    trade.exit_price = prices.back().close;
    trade.shares = position.shares;
    trade.pnl = pnl;
    trade.costs = entry_cost + final_cost;
    trade.entry_signal_quality = position.entry_signal_quality;
    trade.holding_days = position.held_days;
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
    const SimulationRules& rules,
    const ExecutionConfig& execution) {
  return run_simulation(prices, strategy, rules, execution.initial_equity, execution.costs, execution.annualization);
}

}  // namespace metis
