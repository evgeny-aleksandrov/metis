#include "metis/strategy/regime_strategy.hpp"

#include "metis/analytics/indicators.hpp"
#include "metis/strategy/strategy_type.hpp"

#include <algorithm>
#include <variant>

namespace metis {
namespace {

const DiscreteGridStrategyParams& regime_params(const StrategyParams& params) {
  return std::get<DiscreteGridStrategyParams>(params);
}

}  // namespace

RegimeStrategy::RegimeStrategy(DiscreteGridStrategyParams params) : params_(params) {}

std::string RegimeStrategy::name() const {
  const DiscreteGridStrategyParams& params = regime_params(params_);
  return discrete_grid_strategy_to_string(params.strategy);
}

const StrategyParams& RegimeStrategy::params() const {
  return params_;
}

int RegimeStrategy::required_lookback() const {
  const DiscreteGridStrategyParams& params = regime_params(params_);
  int lookback = std::max(params.lookback_days, params.fast_lookback_days);
  if (params.allow_short) {
    lookback = std::max(lookback, std::max(params.short_lookback_days, params.short_fast_lookback_days));
  }
  return lookback;
}

bool RegimeStrategy::should_exit_on_signal_weakness(
    const std::vector<Candle>& prices,
    size_t index,
    int position_direction) const {
  const DiscreteGridStrategyParams& params = regime_params(params_);
  if (!params.exit_on_regime_weakness) {
    return false;
  }
  if (position_direction > 0) {
    const double fast_now = simple_moving_average(prices, index, params.fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params.lookback_days);
    return fast_now > 0.0 && slow_now > 0.0 && fast_now < slow_now;
  }
  if (position_direction < 0) {
    const double fast_now = simple_moving_average(prices, index, params.short_fast_lookback_days);
    const double slow_now = simple_moving_average(prices, index, params.short_lookback_days);
    return fast_now > 0.0 && slow_now > 0.0 && fast_now > slow_now;
  }
  return false;
}

Signal RegimeStrategy::signal(
    const std::vector<Candle>& prices,
    size_t index) const {
  const DiscreteGridStrategyParams& params = regime_params(params_);
  Signal result;
  result.target_position_pct = std::max(0.0, params.max_position_pct);

  const double fast_now = simple_moving_average(prices, index, params.fast_lookback_days);
  const double slow_now = simple_moving_average(prices, index, params.lookback_days);
  const double fast_prev = index > 0 ? simple_moving_average(prices, index - 1, params.fast_lookback_days) : 0.0;
  const double slow_prev = index > 0 ? simple_moving_average(prices, index - 1, params.lookback_days) : 0.0;
  if (fast_prev > 0.0 && slow_prev > 0.0 && fast_now > 0.0 && slow_now > 0.0 &&
      fast_prev <= slow_prev && fast_now > slow_now) {
    result.direction = Direction::Long;
    result.confidence = (fast_now - slow_now) / slow_now;
    return result;
  }

  if (params.allow_short) {
    const double short_fast_now = simple_moving_average(prices, index, params.short_fast_lookback_days);
    const double short_slow_now = simple_moving_average(prices, index, params.short_lookback_days);
    const double short_fast_prev =
        index > 0 ? simple_moving_average(prices, index - 1, params.short_fast_lookback_days) : 0.0;
    const double short_slow_prev =
        index > 0 ? simple_moving_average(prices, index - 1, params.short_lookback_days) : 0.0;
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

}  // namespace metis
