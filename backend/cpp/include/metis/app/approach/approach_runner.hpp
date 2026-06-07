#pragma once

#include "metis/app/backtester_workflow.hpp"
#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

class ApproachRunner {
public:
  virtual ~ApproachRunner() = default;

  virtual BacktestExecutionResult run(
      const BacktestRunConfig& config,
      const std::vector<Candle>& prices) const = 0;
};

}  // namespace metis
