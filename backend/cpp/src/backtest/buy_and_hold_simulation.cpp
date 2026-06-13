#include "metis/backtest/simulation.hpp"

#include "metis/backtest/execution_accounting.hpp"
#include "metis/backtest/metrics.hpp"

#include <algorithm>
#include <vector>

namespace metis {

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
  const double denominator = entry_price * (1.0 + std::max(0.0, costs.variable_rate));
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
  trade.holding_days = static_cast<int>(prices.size() - 1);
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

}  // namespace metis
