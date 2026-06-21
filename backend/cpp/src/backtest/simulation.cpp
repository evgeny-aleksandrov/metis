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
namespace {

struct SimulationState {
  double cash = 0.0;
  double entry_cost = 0.0;
  int wins = 0;
  int trades = 0;
  OpenPosition position;
};

void validate_rules(const SimulationRules& rules) {
  if (!rules.sizing_policy) {
    throw std::runtime_error("SimulationRules must include a position sizing policy.");
  }
  if (!rules.exit_policy) {
    throw std::runtime_error("SimulationRules must include a trade exit policy.");
  }
}

int required_lookback_for(const Strategy& strategy, const SimulationRules& rules) {
  return std::max(strategy.required_lookback(), rules.sizing_policy->required_lookback());
}

int direction_from_signal(const Signal& signal) {
  if (signal.direction == Direction::Long) {
    return 1;
  }
  if (signal.direction == Direction::Short) {
    return -1;
  }
  return 0;
}

double position_equity(const OpenPosition& position, double cash, double close) {
  return cash + static_cast<double>(position.direction) * position.shares * close;
}

void append_trade(
    SimulationResult& result,
    const OpenPosition& position,
    const std::string& exit_date,
    double exit_price,
    double pnl,
    double total_costs) {
  Trade trade;
  trade.entry_date = position.entry_date;
  trade.exit_date = exit_date;
  trade.direction = position.direction > 0 ? "long" : "short";
  trade.entry_price = position.entry_price;
  trade.exit_price = exit_price;
  trade.shares = position.shares;
  trade.pnl = pnl;
  trade.costs = total_costs;
  trade.entry_signal_quality = position.entry_signal_quality;
  trade.holding_days = position.held_days;
  result.trades.push_back(trade);
}

void close_position(
    SimulationState& state,
    SimulationResult& result,
    const Candle& candle,
    const TransactionCosts& costs) {
  const double exit_value = state.position.shares * candle.close;
  const double exit_cost = order_cost(exit_value, costs);
  if (state.position.direction > 0) {
    state.cash += std::max(0.0, exit_value - exit_cost);
  } else {
    state.cash -= exit_value + exit_cost;
  }

  const double pnl = state.cash - state.position.entry_value;
  if (pnl > 0.0) {
    state.wins += 1;
  }
  append_trade(
      result,
      state.position,
      candle.date,
      candle.close,
      pnl,
      state.entry_cost + exit_cost);
  state.position = {};
}

void exit_position_if_needed(
    SimulationState& state,
    SimulationResult& result,
    const std::vector<Candle>& prices,
    size_t index,
    const SimulationRules& rules,
    const TransactionCosts& costs) {
  if (!state.position.is_open()) {
    return;
  }

  state.position.held_days += 1;
  const ExitDecision exit = rules.exit_policy->evaluate(prices, index, state.position);
  if (exit.should_exit) {
    close_position(state, result, prices[index], costs);
  }
}

bool enter_position_if_signaled(
    SimulationState& state,
    const std::vector<Candle>& prices,
    size_t index,
    const Strategy& strategy,
    const SimulationRules& rules,
    const TransactionCosts& costs,
    const Annualization& annualization) {
  if (state.position.is_open()) {
    return false;
  }

  const Signal signal = strategy.signal(prices, index);
  const int entry_direction = direction_from_signal(signal);
  if (entry_direction == 0) {
    return false;
  }

  const double equity = state.cash;
  const double fraction = rules.sizing_policy->target_fraction(
      prices,
      index,
      equity,
      annualization);
  const double target_notional = equity * std::max(0.0, fraction);
  const double entry_cost_budget = std::min(state.cash, target_notional) - costs.fixed_per_order;
  const double close = prices[index].close;
  const double denominator = close * (1.0 + std::max(0.0, costs.variable_rate));
  if (entry_cost_budget <= 0.0 || denominator <= 0.0) {
    return false;
  }

  state.position.shares = entry_cost_budget / denominator;
  const double entry_trade_value = state.position.shares * close;
  state.position.entry_value = equity;
  state.position.entry_price = close;
  state.entry_cost = order_cost(entry_trade_value, costs);
  state.position.entry_date = prices[index].date;
  state.position.direction = entry_direction;
  state.position.entry_signal_quality = signal.confidence;
  if (state.position.direction > 0) {
    state.cash -= entry_trade_value + state.entry_cost;
  } else {
    state.cash += entry_trade_value - state.entry_cost;
  }
  state.position.held_days = 0;
  rules.exit_policy->on_entry(state.position);
  state.trades += 1;
  return true;
}

void record_equity(
    SimulationResult& result,
    std::vector<double>& equity_curve,
    const SimulationState& state,
    const Candle& candle) {
  const double equity = position_equity(state.position, state.cash, candle.close);
  equity_curve.push_back(equity);
  result.equity_curve.push_back({candle.date, equity});
}

void compute_metrics(
    SimulationResult& result,
    const std::vector<double>& equity_curve,
    double initial_equity,
    size_t price_count,
    const Annualization& annualization,
    int trades,
    int wins) {
  result.metrics.total_return = compute_total_return(initial_equity, result.final_equity);
  result.metrics.cagr = compute_cagr_from_bars(
      initial_equity,
      result.final_equity,
      price_count,
      annualization.bars_per_year);
  result.metrics.max_drawdown = compute_max_drawdown(equity_curve);
  result.metrics.sharpe = compute_sharpe(equity_curve, annualization.bars_per_year);
  result.metrics.sortino = compute_sortino(equity_curve, annualization.bars_per_year);
  result.metrics.trades = trades;
  result.metrics.win_rate = (trades > 0) ? (static_cast<double>(wins) / static_cast<double>(trades)) : 0.0;
}

}  // namespace

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

  validate_rules(rules);

  const int required_lookback = required_lookback_for(strategy, rules);
  if (prices.size() < static_cast<size_t>(required_lookback + 2)) {
    return result;
  }

  SimulationState state;
  state.cash = initial_equity;

  std::vector<double> equity_curve;
  equity_curve.reserve(prices.size());
  result.equity_curve.reserve(prices.size());

  for (size_t index = 0; index < prices.size(); ++index) {
    exit_position_if_needed(state, result, prices, index, rules, costs);

    const bool has_lookback = index >= static_cast<size_t>(required_lookback);
    if (has_lookback) {
      enter_position_if_signaled(state, prices, index, strategy, rules, costs, annualization);
    }

    record_equity(result, equity_curve, state, prices[index]);
  }

  if (state.position.is_open()) {
    close_position(state, result, prices.back(), costs);
  }
  result.final_equity = state.cash;

  compute_metrics(
      result,
      equity_curve,
      initial_equity,
      prices.size(),
      annualization,
      state.trades,
      state.wins);

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
