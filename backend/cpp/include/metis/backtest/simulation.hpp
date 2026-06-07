#pragma once

#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

SimulationResult run_simulation(
    const std::vector<Candle>& prices,
    const StrategyParams& params,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});

SimulationResult run_buy_and_hold(
    const std::vector<Candle>& prices,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});

}  // namespace metis
