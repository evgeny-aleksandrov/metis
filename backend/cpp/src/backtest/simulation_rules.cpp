#include "metis/backtest/simulation_rules.hpp"

#include <variant>

namespace metis {

SimulationRules simulation_rules_from(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy) {
  SimulationRules rules;
  if (params.target_volatility > 0.0 && params.volatility_lookback_days > 1) {
    rules.sizing_policy = std::make_unique<VolTargetSizingPolicy>(
        params.max_position_pct,
        params.target_volatility,
        params.volatility_lookback_days);
  } else {
    rules.sizing_policy = std::make_unique<FixedFractionSizingPolicy>(params.max_position_pct);
  }

  auto exit_policy = std::make_unique<CompositeExitPolicy>();
  if (params.strategy != DiscreteGridStrategy::Regime && params.hold_days > 0) {
    exit_policy->add(std::make_unique<HoldPeriodExitPolicy>(params.hold_days));
  }
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
  if (params.strategy == DiscreteGridStrategy::Regime && params.exit_on_regime_weakness) {
    exit_policy->add(std::make_unique<SignalWeaknessExitPolicy>(strategy));
  }
  rules.exit_policy = std::move(exit_policy);
  return rules;
}

SimulationRules simulation_rules_from(
    const StrategyParams& params,
    const Strategy& strategy) {
  if (const auto* discrete_grid_params = std::get_if<DiscreteGridStrategyParams>(&params)) {
    return simulation_rules_from(*discrete_grid_params, strategy);
  }
  SimulationRules rules;
  rules.sizing_policy = std::make_unique<FixedFractionSizingPolicy>(1.0);
  rules.exit_policy = std::make_unique<CompositeExitPolicy>();
  return rules;
}

}  // namespace metis
