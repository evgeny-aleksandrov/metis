#pragma once

#include "metis/config/run_config.hpp"
#include "metis/optimization/optimizer.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

class DiscreteGridSearchOptimizer final : public Optimizer {
public:
  explicit DiscreteGridSearchOptimizer(DiscreteStrategySearchConfig config);

  std::vector<SimulationResult> evaluate(
      const std::vector<Candle>& prices,
      const ExecutionConfig& execution) const override;

private:
  DiscreteStrategySearchConfig config_;
};

std::vector<SimulationResult> run_grid_search(
    const std::vector<Candle>& prices,
    const GridSearchConfig& config,
    StrategyType strategy,
    const ExecutionConfig& execution);

}  // namespace metis
