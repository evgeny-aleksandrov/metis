#include "metis/strategy/ridge_strategy.hpp"

#include <variant>

namespace metis {
namespace {

const RidgeStrategyParams& ridge_params(const StrategyParams& params) {
  return std::get<RidgeStrategyParams>(params);
}

}  // namespace

RidgeStrategyModel::RidgeStrategyModel(RidgeStrategyParams params) : params_(params) {}

std::string RidgeStrategyModel::name() const {
  return "ridge";
}

const StrategyParams& RidgeStrategyModel::params() const {
  return params_;
}

int RidgeStrategyModel::required_lookback() const {
  const RidgeStrategyParams& params = ridge_params(params_);
  return params.feature_lookback_days;
}

bool RidgeStrategyModel::should_exit_on_signal_weakness(
    const std::vector<Candle>& /*prices*/,
    size_t /*index*/,
    int /*position_direction*/) const {
  return false;
}

Signal RidgeStrategyModel::signal(
    const std::vector<Candle>& /*prices*/,
    size_t /*index*/) const {
  return {};
}

}  // namespace metis
