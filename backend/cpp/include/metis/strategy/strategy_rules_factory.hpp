#pragma once

#include "metis/backtest/simulation_rules.hpp"
#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

namespace metis {

SimulationRules make_strategy_rules(
    const DiscreteGridStrategyParams& params,
    const Strategy& strategy);
SimulationRules make_strategy_rules(
    const RidgeStrategyParams& params,
    const Strategy& strategy);
SimulationRules make_strategy_rules(
    const StrategyParams& params,
    const Strategy& strategy);

}  // namespace metis
