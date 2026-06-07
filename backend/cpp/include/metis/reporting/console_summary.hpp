#pragma once

#include "metis/cli/cli_config.hpp"
#include "metis/types.hpp"

#include <cstddef>

namespace metis {

void print_summary(
    const CliConfig& config,
    size_t rows_loaded,
    const SimulationResult& buy_and_hold,
    const SimulationResult& summary,
    size_t walk_periods);

}  // namespace metis
