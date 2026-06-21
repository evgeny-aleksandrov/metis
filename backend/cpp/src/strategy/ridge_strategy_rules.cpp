#include "metis/strategy/ridge_strategy_rules.hpp"

namespace metis {

SimulationRules make_ridge_strategy_rules(
    const RidgeStrategyParams& /*params*/,
    const Strategy& /*strategy*/) {
  SimulationRules rules;
  rules.sizing_policy = std::make_unique<FixedFractionSizingPolicy>(1.0);
  rules.exit_policy = std::make_unique<CompositeExitPolicy>();
  return rules;
}

}  // namespace metis
