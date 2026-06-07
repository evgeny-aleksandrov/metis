#include "metis/strategy/strategy_type.hpp"

#include <stdexcept>

namespace metis {

StrategyType strategy_type_from_string(const std::string& value) {
  if (value == "drop" || value == "dip") {
    return StrategyType::Drop;
  }
  if (value == "gain" || value == "momentum") {
    return StrategyType::Gain;
  }
  if (value == "regime" || value == "trend") {
    return StrategyType::Regime;
  }
  throw std::runtime_error("Unknown strategy type: " + value + ". Use 'drop', 'gain', or 'regime'.");
}

std::string strategy_type_to_string(StrategyType strategy) {
  if (strategy == StrategyType::Gain) {
    return "gain";
  }
  if (strategy == StrategyType::Regime) {
    return "regime";
  }
  return "drop";
}

}  // namespace metis
