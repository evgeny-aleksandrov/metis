#include "metis/analytics/indicators.hpp"
#include "metis/strategy/strategy.hpp"

#include "metis/strategy/strategy_type.hpp"

#include <algorithm>

//TODO: Wire this in properly

namespace metis {

DiscreteStrategy::DiscreteStrategy(StrategyParams params) : params_(params) {}

std::string DiscreteStrategy::name() const {
  return discrete_grid_strategy_to_string(params_.strategy);
}

const StrategyParams& DiscreteStrategy::params() const {
  return params_;
}

int DiscreteStrategy::required_lookback() const {
  int lookback =
      params_.strategy == DiscreteGridStrategy::Regime ? std::max(params_.lookback_days, params_.fast_lookback_days) : params_.lookback_days;
  if (params_.strategy == DiscreteGridStrategy::Regime && params_.allow_short) {
    lookback = std::max(lookback, std::max(params_.short_lookback_days, params_.short_fast_lookback_days));
  }
  return lookback;
}

bool DiscreteStrategy::uses_timed_exit() const {
  return params_.strategy != DiscreteGridStrategy::Regime;
}

bool DiscreteStrategy::should_exit_on_signal_weakness(
    const std::vector<Candle>& prices,
    size_t index,
    int position_direction) const {
  if (params_.strategy != DiscreteGridStrategy::Regime || !params_.exit_on_regime_weakness) {
    return false;
  }
  if (position_direction > 0) {
    const double fast_now = simple_moving_average(prices, index, params_.fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params_.lookback_days);
    return fast_now > 0.0 && slow_now > 0.0 && fast_now < slow_now;
  }
  if (position_direction < 0) {
    const double fast_now = simple_moving_average(prices, index, params_.short_fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params_.short_lookback_days);
    return fast_now > 0.0 && slow_now > 0.0 && fast_now > slow_now;
  }
  return false;
}

Signal DiscreteStrategy::signal(
    const std::vector<Candle>& prices,
    size_t index,
    const PortfolioState& /*state*/) const {
  Signal result;
  result.target_position_pct = std::max(0.0, params_.max_position_pct);

  if (params_.strategy == DiscreteGridStrategy::Regime) {
    const double fast_now = simple_moving_average(prices, index, params_.fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params_.lookback_days);
    const double fast_prev = index > 0 ? simple_moving_average(prices, index - 1, params_.fast_lookback_days) : 0.0;
    const double slow_prev = index > 0 ? simple_moving_average(prices, index - 1, params_.lookback_days) : 0.0;
    if (fast_prev > 0.0 && slow_prev > 0.0 && fast_now > 0.0 && slow_now > 0.0 &&
        fast_prev <= slow_prev && fast_now > slow_now) {
      result.direction = Direction::Long;
      result.confidence = (fast_now - slow_now) / slow_now;
      return result;
    }
    if (params_.allow_short) {
      const double short_fast_now = simple_moving_average(prices, index, params_.short_fast_lookback_days);
      const double short_slow_now = simple_moving_average(prices, index, params_.short_lookback_days);
      const double short_fast_prev = index > 0 ? simple_moving_average(prices, index - 1, params_.short_fast_lookback_days) : 0.0;
      const double short_slow_prev = index > 0 ? simple_moving_average(prices, index - 1, params_.short_lookback_days) : 0.0;
      if (short_fast_prev > 0.0 && short_slow_prev > 0.0 && short_fast_now > 0.0 &&
          short_slow_now > 0.0 && short_fast_prev >= short_slow_prev &&
          short_fast_now < short_slow_now) {
        result.direction = Direction::Short;
        result.confidence = (short_slow_now - short_fast_now) / short_slow_now;
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
  if (params_.strategy == DiscreteGridStrategy::Drop && diff <= -threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  } else if (params_.strategy == DiscreteGridStrategy::Gain && diff >= threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  }
  return result;
}

}  // namespace metis
