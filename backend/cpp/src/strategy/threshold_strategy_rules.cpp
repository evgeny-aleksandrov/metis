#include "metis/strategy/threshold_strategy_rules.hpp"

namespace metis {
namespace {

std::unique_ptr<PositionSizingPolicy> make_sizing_policy(
    const DiscreteGridStrategyParams& params) {
  return std::make_unique<FixedFractionSizingPolicy>(params.max_position_pct);
}

std::unique_ptr<CompositeExitPolicy> make_exit_policy(
    const DiscreteGridStrategyParams& params) {
  auto exit_policy = std::make_unique<CompositeExitPolicy>();
  if (params.hold_days > 0) {
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
  return exit_policy;
}

}  // namespace

SimulationRules make_threshold_strategy_rules(
    const DiscreteGridStrategyParams& params,
    const Strategy& /*strategy*/) {
  SimulationRules rules;
  rules.sizing_policy = make_sizing_policy(params);
  rules.exit_policy = make_exit_policy(params);
  return rules;
}

}  // namespace metis
