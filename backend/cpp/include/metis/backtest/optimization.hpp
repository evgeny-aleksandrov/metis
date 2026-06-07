#pragma once

#include "metis/types.hpp"

#include <vector>

namespace metis {

std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    double initial_equity,
    const TransactionCosts& costs = {},
    const Annualization& annualization = {});

}  // namespace metis
