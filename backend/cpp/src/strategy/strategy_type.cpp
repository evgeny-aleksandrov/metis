#include "metis/strategy/strategy_type.hpp"

#include <stdexcept>

namespace metis {

DiscreteGridStrategy discrete_grid_strategy_from_string(const std::string& value) {
  if (value == "drop" || value == "dip") {
    return DiscreteGridStrategy::Drop;
  }
  if (value == "gain" || value == "momentum") {
    return DiscreteGridStrategy::Gain;
  }
  if (value == "regime" || value == "trend") {
    return DiscreteGridStrategy::Regime;
  }
  throw std::runtime_error("Unknown strategy type: " + value + ". Use 'drop', 'gain', or 'regime'.");
}

std::string discrete_grid_strategy_to_string(DiscreteGridStrategy strategy) {
  if (strategy == DiscreteGridStrategy::Gain) {
    return "gain";
  }
  if (strategy == DiscreteGridStrategy::Regime) {
    return "regime";
  }
  return "drop";
}

}  // namespace metis
