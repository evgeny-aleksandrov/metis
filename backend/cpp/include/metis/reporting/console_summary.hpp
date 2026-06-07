#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <cstddef>

namespace metis {

void print_summary(
    const BacktestRunConfig& config,
    size_t rows_loaded,
    const SimulationResult& buy_and_hold,
    const SimulationResult& summary,
    size_t walk_periods);

}  // namespace metis
