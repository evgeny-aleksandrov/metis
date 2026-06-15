#pragma once

#include "metis/backtest/position_sizing_policy.hpp"
#include "metis/backtest/trade_exit_policy.hpp"
#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

#include <memory>

namespace metis {

struct SimulationRules {
  std::unique_ptr<PositionSizingPolicy> sizing_policy;
  std::unique_ptr<TradeExitPolicy> exit_policy;
};

SimulationRules simulation_rules_from(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy);
SimulationRules simulation_rules_from(
    const StrategyParams& params,
    const Strategy& strategy);

}  // namespace metis
