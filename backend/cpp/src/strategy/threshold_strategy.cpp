#include "metis/strategy/threshold_strategy.hpp"

#include "metis/strategy/strategy_type.hpp"

#include <algorithm>
#include <cmath>
#include <variant>

namespace metis {
namespace {

const DiscreteGridStrategyParams& threshold_params(const StrategyParams& params) {
  return std::get<DiscreteGridStrategyParams>(params);
}

}  // namespace

ThresholdStrategy::ThresholdStrategy(DiscreteGridStrategyParams params) : params_(params) {}

std::string ThresholdStrategy::name() const {
  const DiscreteGridStrategyParams& params = threshold_params(params_);
  return discrete_grid_strategy_to_string(params.strategy);
}

const StrategyParams& ThresholdStrategy::params() const {
  return params_;
}

int ThresholdStrategy::required_lookback() const {
  const DiscreteGridStrategyParams& params = threshold_params(params_);
  return params.lookback_days;
}

bool ThresholdStrategy::should_exit_on_signal_weakness(
    const std::vector<Candle>& /*prices*/,
    size_t /*index*/,
    int /*position_direction*/) const {
  return false;
}

Signal ThresholdStrategy::signal(
    const std::vector<Candle>& prices,
    size_t index) const {
  const DiscreteGridStrategyParams& params = threshold_params(params_);
  Signal result;
  result.target_position_pct = std::max(0.0, params.max_position_pct);

  if (index < static_cast<size_t>(params.lookback_days)) {
    return result;
  }
  const double previous = prices[index - static_cast<size_t>(params.lookback_days)].close;
  if (previous <= 0.0) {
    return result;
  }

  const double diff = (prices[index].close / previous) - 1.0;
  const double threshold = std::abs(params.diff_pct);
  if (params.strategy == DiscreteGridStrategy::Drop && diff <= -threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  } else if (params.strategy == DiscreteGridStrategy::Gain && diff >= threshold) {
    result.direction = Direction::Long;
    result.confidence = std::abs(diff);
  }
  return result;
}

}  // namespace metis
