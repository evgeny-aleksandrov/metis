#pragma once

#include "metis/app/approach/approach_runner.hpp"

namespace metis {

class RidgeRunner final : public ApproachRunner {
public:
  BacktestExecutionResult run(
      const BacktestRunConfig& config,
      const std::vector<Candle>& prices) const override;
};

}  // namespace metis
