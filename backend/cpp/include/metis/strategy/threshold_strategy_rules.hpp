#pragma once

#include "metis/backtest/simulation_rules.hpp"
#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

namespace metis {

SimulationRules make_threshold_strategy_rules(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy);

}  // namespace metis
