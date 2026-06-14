#pragma once

#include "metis/types.hpp"

namespace metis {

struct PositionSizingRules {
  double max_position_pct = 1.0;
  double target_volatility = 0.0;
  int volatility_lookback_days = 0;
};

struct TradeManagementRules {
  int hold_days = 0;
  double take_profit_pct = 0.0;
  double stop_loss_pct = 0.0;
  double trailing_stop_pct = 0.0;
  bool use_timed_exit = false;
  bool use_signal_weakness_exit = false;
};

struct SimulationRules {
  PositionSizingRules sizing;
  TradeManagementRules trade_management;
};

SimulationRules simulation_rules_from(const DiscreteGridStrategyParams& params);
SimulationRules simulation_rules_from(const StrategyParams& params);

}  // namespace metis
