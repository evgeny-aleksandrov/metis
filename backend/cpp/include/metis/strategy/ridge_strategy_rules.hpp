#pragma once

#include "metis/backtest/simulation_rules.hpp"
#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

namespace metis {

SimulationRules make_ridge_strategy_rules(
    const RidgeStrategyParams& params,
    const Strategy& strategy);

}  // namespace metis
