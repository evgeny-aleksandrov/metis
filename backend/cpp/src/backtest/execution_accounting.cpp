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
    double max_position_pct,
    double target_volatility,
    int volatility_lookback_days,
    const Annualization& annualization) {
  const double position_cap = std::max(0.0, max_position_pct);
  if (target_volatility <= 0.0 || volatility_lookback_days <= 1) {
    return position_cap;
  }
  const double vol = realized_volatility(prices, index, volatility_lookback_days, annualization.bars_per_year);
  if (vol <= 0.0) {
    return 0.0;
  }
  return std::min(position_cap, target_volatility / vol);
}

}  // namespace metis
