#include "metis/backtest/position_sizing_policy.hpp"

#include "metis/analytics/indicators.hpp"

#include <algorithm>

namespace metis {

FixedFractionSizingPolicy::FixedFractionSizingPolicy(double max_position_pct)
    : max_position_pct_(max_position_pct) {}

int FixedFractionSizingPolicy::required_lookback() const {
  return 0;
}

double FixedFractionSizingPolicy::target_fraction(
    const std::vector<Candle>& /*prices*/,
    size_t /*index*/,
    double /*equity*/,
    const Annualization& /*annualization*/) const {
  return std::max(0.0, max_position_pct_);
}

VolTargetSizingPolicy::VolTargetSizingPolicy(
    double max_position_pct,
    double target_volatility,
    int volatility_lookback_days)
    : max_position_pct_(max_position_pct),
      target_volatility_(target_volatility),
      volatility_lookback_days_(volatility_lookback_days) {}

int VolTargetSizingPolicy::required_lookback() const {
  return volatility_lookback_days_;
}

double VolTargetSizingPolicy::target_fraction(
    const std::vector<Candle>& prices,
    size_t index,
    double /*equity*/,
    const Annualization& annualization) const {
  const double position_cap = std::max(0.0, max_position_pct_);
  if (target_volatility_ <= 0.0 || volatility_lookback_days_ <= 1) {
    return position_cap;
  }
  const double vol = realized_volatility(prices, index, volatility_lookback_days_, annualization.bars_per_year);
  if (vol <= 0.0) {
    return 0.0;
  }
  return std::min(position_cap, target_volatility_ / vol);
}

}  // namespace metis
