#pragma once

#include "metis/config/run_config.hpp"
#include "metis/optimization/discrete_grid_search.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    const ExecutionConfig& execution);

}  // namespace metis
