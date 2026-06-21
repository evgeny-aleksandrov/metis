#include "metis/strategy/strategy_factory.hpp"

#include "metis/strategy/regime_strategy.hpp"
#include "metis/strategy/ridge_strategy.hpp"
#include "metis/strategy/threshold_strategy.hpp"

#include <stdexcept>
#include <variant>

namespace metis {

std::unique_ptr<Strategy> make_strategy(const DiscreteGridStrategyParams& params) {
  if (params.strategy == DiscreteGridStrategy::Regime) {
    return std::make_unique<RegimeStrategy>(params);
  }
  if (params.strategy == DiscreteGridStrategy::Drop || params.strategy == DiscreteGridStrategy::Gain) {
    return std::make_unique<ThresholdStrategy>(params);
  }
  throw std::runtime_error("Unsupported discrete-grid strategy.");
}

std::unique_ptr<Strategy> make_strategy(const RidgeStrategyParams& params) {
  return std::make_unique<RidgeStrategyModel>(params);
}

std::unique_ptr<Strategy> make_strategy(const StrategyParams& params) {
  if (const auto* discrete_params = std::get_if<DiscreteGridStrategyParams>(&params)) {
    return make_strategy(*discrete_params);
  }
  if (const auto* ridge_params = std::get_if<RidgeStrategyParams>(&params)) {
    return make_strategy(*ridge_params);
  }
  throw std::runtime_error("Unsupported strategy params.");
}

}  // namespace metis
