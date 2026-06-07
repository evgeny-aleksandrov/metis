#pragma once

#include "metis/config/run_config.hpp"
#include "metis/optimization/optimizer.hpp"
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
    const BacktestRunConfig& config,
    const Optimizer& optimizer,
    std::vector<WalkForwardPeriod>& periods);

}  // namespace metis
