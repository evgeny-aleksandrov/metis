#pragma once

#include "metis/backtest/walk_forward.hpp"
#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

struct BacktestExecutionResult {
  SimulationResult benchmark;
  SimulationResult summary;
  std::vector<WalkForwardPeriod> walk_forward_periods;
  bool has_walk_forward = false;
};

class BacktesterWorkflow {
public:
  BacktestExecutionResult run(
      const BacktestRunConfig& config,
      const std::vector<Candle>& prices) const;
};

}  // namespace metis
