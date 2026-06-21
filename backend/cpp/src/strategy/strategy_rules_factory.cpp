#include "metis/strategy/strategy_rules_factory.hpp"

#include "metis/strategy/regime_strategy_rules.hpp"
#include "metis/strategy/ridge_strategy_rules.hpp"
#include "metis/strategy/threshold_strategy_rules.hpp"

#include <stdexcept>
#include <variant>

namespace metis {

SimulationRules make_strategy_rules(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy) {
  if (params.strategy == DiscreteGridStrategy::Regime) {
    return make_regime_strategy_rules(params, strategy);
  }
  if (params.strategy == DiscreteGridStrategy::Drop || params.strategy == DiscreteGridStrategy::Gain) {
    return make_threshold_strategy_rules(params, strategy);
  }
  throw std::runtime_error("Unsupported discrete-grid strategy rules.");
}

SimulationRules make_strategy_rules(
    const RidgeStrategyParams& params,
    const Strategy& strategy) {
  return make_ridge_strategy_rules(params, strategy);
}

SimulationRules make_strategy_rules(
    const StrategyParams& params,
    const Strategy& strategy) {
  if (const auto* discrete_params = std::get_if<DiscreteGridStrategyParams>(&params)) {
    return make_strategy_rules(*discrete_params, strategy);
  }
  if (const auto* ridge_params = std::get_if<RidgeStrategyParams>(&params)) {
    return make_strategy_rules(*ridge_params, strategy);
  }
  throw std::runtime_error("Unsupported strategy rules params.");
}

}  // namespace metis
