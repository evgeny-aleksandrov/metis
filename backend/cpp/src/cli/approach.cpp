#include "metis/cli/approach.hpp"

#include <stdexcept>

namespace metis {

ApproachType approach_type_from_string(const std::string& value) {
  if (value == "buy-and-hold" || value == "buyhold" || value == "bnh") {
    return ApproachType::BuyAndHold;
  }
  if (value == "discrete-grid" || value == "grid") {
    return ApproachType::DiscreteGrid;
  }
  if (value == "ridge" || value == "ridge-regression") {
    return ApproachType::Ridge;
  }
  throw std::runtime_error("Unknown approach: " + value + ". Use 'buy-and-hold', 'discrete-grid', or 'ridge'.");
}

std::string approach_type_to_string(ApproachType approach) {
  if (approach == ApproachType::BuyAndHold) {
    return "buy-and-hold";
  }
  if (approach == ApproachType::Ridge) {
    return "ridge";
  }
  return "discrete-grid";
}

StrategyFamily strategy_family_from_string(ApproachType approach, const std::string& value) {
  if (approach == ApproachType::BuyAndHold) {
    if (value == "lump-sum" || value == "lumpsum") {
      return StrategyFamily::LumpSumBuyAndHold;
    }
    if (value == "scheduled-buy" || value == "monthly-buy") {
      return StrategyFamily::ScheduledBuy;
    }
    throw std::runtime_error("Unknown buy-and-hold strategy: " + value + ". Use 'lump-sum'.");
  }
  if (approach == ApproachType::DiscreteGrid) {
    if (value == "drop" || value == "dip") {
      return StrategyFamily::Drop;
    }
    if (value == "gain" || value == "momentum") {
      return StrategyFamily::Gain;
    }
    if (value == "regime" || value == "trend") {
      return StrategyFamily::Regime;
    }
    throw std::runtime_error("Unknown discrete-grid strategy: " + value + ". Use 'drop', 'gain', or 'regime'.");
  }
  if (approach == ApproachType::Ridge) {
    if (value == "directional") {
      return StrategyFamily::RidgeDirectional;
    }
    if (value == "long-short") {
      return StrategyFamily::RidgeLongShort;
    }
    if (value == "vol-target") {
      return StrategyFamily::RidgeVolTarget;
    }
    throw std::runtime_error("Unknown ridge strategy: " + value + ". Use 'directional', 'long-short', or 'vol-target'.");
  }
  throw std::runtime_error("Unsupported approach for strategy parsing: " + approach_type_to_string(approach));
}

std::string strategy_family_to_string(StrategyFamily strategy) {
  if (strategy == StrategyFamily::LumpSumBuyAndHold) {
    return "lump-sum";
  }
  if (strategy == StrategyFamily::ScheduledBuy) {
    return "scheduled-buy";
  }
  if (strategy == StrategyFamily::Gain) {
    return "gain";
  }
  if (strategy == StrategyFamily::Regime) {
    return "regime";
  }
  if (strategy == StrategyFamily::RidgeDirectional) {
    return "directional";
  }
  if (strategy == StrategyFamily::RidgeLongShort) {
    return "long-short";
  }
  if (strategy == StrategyFamily::RidgeVolTarget) {
    return "vol-target";
  }
  return "drop";
}

bool is_strategy_supported_by_approach(ApproachType approach, StrategyFamily strategy) {
  if (approach == ApproachType::BuyAndHold) {
    return strategy == StrategyFamily::LumpSumBuyAndHold || strategy == StrategyFamily::ScheduledBuy;
  }
  if (approach == ApproachType::DiscreteGrid) {
    return strategy == StrategyFamily::Drop || strategy == StrategyFamily::Gain || strategy == StrategyFamily::Regime;
  }
  return strategy == StrategyFamily::RidgeDirectional || strategy == StrategyFamily::RidgeLongShort ||
         strategy == StrategyFamily::RidgeVolTarget;
}

StrategyType discrete_strategy_type_from_family(StrategyFamily strategy) {
  if (strategy == StrategyFamily::Gain) {
    return StrategyType::Gain;
  }
  if (strategy == StrategyFamily::Regime) {
    return StrategyType::Regime;
  }
  if (strategy == StrategyFamily::Drop) {
    return StrategyType::Drop;
  }
  throw std::runtime_error("Strategy is not supported by the discrete-grid approach: " + strategy_family_to_string(strategy));
}

}  // namespace metis
