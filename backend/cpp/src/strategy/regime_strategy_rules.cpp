#include "metis/strategy/regime_strategy_rules.hpp"

namespace metis {
namespace {

std::unique_ptr<PositionSizingPolicy> make_sizing_policy(
    const DiscreteGridStrategyParams& params) {
  if (params.target_volatility > 0.0 && params.volatility_lookback_days > 1) {
    return std::make_unique<VolTargetSizingPolicy>(
        params.max_position_pct,
        params.target_volatility,
        params.volatility_lookback_days);
  }
  return std::make_unique<FixedFractionSizingPolicy>(params.max_position_pct);
}

std::unique_ptr<CompositeExitPolicy> make_exit_policy(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy) {
  auto exit_policy = std::make_unique<CompositeExitPolicy>();
  if (params.take_profit_pct > 0.0 && params.trailing_stop_pct <= 0.0) {
    exit_policy->add(std::make_unique<TakeProfitExitPolicy>(params.take_profit_pct));
  }
  if (params.stop_loss_pct > 0.0) {
    exit_policy->add(std::make_unique<StopLossExitPolicy>(params.stop_loss_pct));
  }
  if (params.trailing_stop_pct > 0.0) {
    exit_policy->add(std::make_unique<TrailingStopExitPolicy>(
        params.trailing_stop_pct,
        params.take_profit_pct));
  }
  if (params.exit_on_regime_weakness) {
    exit_policy->add(std::make_unique<SignalWeaknessExitPolicy>(strategy));
  }
  return exit_policy;
}

}  // namespace

SimulationRules make_regime_strategy_rules(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy) {
  SimulationRules rules;
  rules.sizing_policy = make_sizing_policy(params);
  rules.exit_policy = make_exit_policy(params, strategy);
  return rules;
}

}  // namespace metis
