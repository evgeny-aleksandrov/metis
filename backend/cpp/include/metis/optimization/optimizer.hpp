#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

class Optimizer {
public:
  virtual ~Optimizer() = default;

  virtual std::vector<SimulationResult> evaluate(
      const std::vector<Candle>& prices,
      const ExecutionConfig& execution) const = 0;
};

}  // namespace metis
