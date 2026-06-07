#include "metis/analytics/indicators.hpp"
#include "metis/strategy/strategy.hpp"

#include "metis/strategy/strategy_type.hpp"

#include <algorithm>

//TODO: Wire this in properly

namespace metis {

DiscreteStrategy::DiscreteStrategy(StrategyParams params) : params_(params) {}

std::string DiscreteStrategy::name() const {
  return strategy_type_to_string(params_.strategy);
}

Signal DiscreteStrategy::signal(
    const std::vector<Candle>& prices,
    size_t index,
    const PortfolioState& /*state*/) const {
  Signal result;
  result.target_position_pct = std::max(0.0, params_.max_position_pct);

  if (params_.strategy == StrategyType::Regime) {
    const double fast_now = simple_moving_average(prices, index, params_.fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params_.lookback_days);
    if (fast_now > 0.0 && slow_now > 0.0 && fast_now > slow_now) {
      result.direction = Direction::Long;
      result.confidence = (fast_now / slow_now) - 1.0;
      return result;
    }
    if (params_.allow_short) {
      const double short_fast_now = simple_moving_average(prices, index, params_.short_fast_lookback_days);
      const double short_slow_now = simple_moving_average(prices, index, params_.short_lookback_days);
      if (short_fast_now > 0.0 && short_slow_now > 0.0 && short_fast_now < short_slow_now) {
        result.direction = Direction::Short;
        result.confidence = (short_slow_now / short_fast_now) - 1.0;
        return result;
      }
    }
    return result;
  }

  if (index < static_cast<size_t>(params_.lookback_days)) {
    return result;
  }
  const double previous = prices[index - static_cast<size_t>(params_.lookback_days)].close;
  if (previous <= 0.0) {
    return result;
  }
  const double diff = (prices[index].close / previous) - 1.0;
  const double threshold = std::abs(params_.diff_pct);
  if (params_.strategy == StrategyType::Drop && diff <= -threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  } else if (params_.strategy == StrategyType::Gain && diff >= threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  }
  return result;
}

const StrategyParams& DiscreteStrategy::params() const {
  return params_;
}

}  // namespace metis
