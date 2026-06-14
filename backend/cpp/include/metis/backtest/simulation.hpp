#pragma once

#include "metis/config/run_config.hpp"
#include "metis/backtest/simulation_rules.hpp"
#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const Strategy& strategy,
    const SimulationRules& rules,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});

SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const Strategy& strategy,
    const SimulationRules& rules,
    const ExecutionConfig& execution);

SimulationResult run_buy_and_hold(
    const std::vector<Candle>& prices,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});

}  // namespace metis
