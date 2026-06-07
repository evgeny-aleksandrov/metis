#pragma once

#include "metis/cli/cli_config.hpp"
#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

struct WalkForwardPeriod {
  std::string train_start;
  std::string train_end;
  std::string test_start;
  std::string test_end;
  SimulationResult training_selection;
  SimulationResult test_result;
};

SimulationResult run_walk_forward(
    const std::vector<Candle>& prices,
    const CliConfig& config,
    std::vector<WalkForwardPeriod>& periods);

}  // namespace metis
