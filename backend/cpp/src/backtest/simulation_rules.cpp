#include "metis/backtest/simulation_rules.hpp"

#include <variant>

namespace metis {

SimulationRules simulation_rules_from(const DiscreteGridStrategyParams& params) {
  SimulationRules rules;
  rules.sizing.max_position_pct = params.max_position_pct;
  rules.sizing.target_volatility = params.target_volatility;
  rules.sizing.volatility_lookback_days = params.volatility_lookback_days;
  rules.trade_management.hold_days = params.hold_days;
  rules.trade_management.take_profit_pct = params.take_profit_pct;
  rules.trade_management.stop_loss_pct = params.stop_loss_pct;
  rules.trade_management.trailing_stop_pct = params.trailing_stop_pct;
  rules.trade_management.use_timed_exit = params.strategy != DiscreteGridStrategy::Regime;
  rules.trade_management.use_signal_weakness_exit =
      params.strategy == DiscreteGridStrategy::Regime &&
      params.exit_on_regime_weakness;
  return rules;
}

SimulationRules simulation_rules_from(const StrategyParams& params) {
  if (const auto* discrete_grid_params = std::get_if<DiscreteGridStrategyParams>(&params)) {
    return simulation_rules_from(*discrete_grid_params);
  }
  return {};
}

}  // namespace metis
