#include "metis/backtest/execution_accounting.hpp"

#include "metis/analytics/indicators.hpp"

#include <algorithm>

namespace metis {

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

}  // namespace metis
