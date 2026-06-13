#include "metis/config/run_config.hpp"

#include <variant>

namespace metis {

std::string approach_name(const BacktestRunConfig& config) {
  if (std::holds_alternative<BuyAndHoldRunConfig>(config.approach_config)) {
    return "buy-and-hold";
  }
  if (std::holds_alternative<RidgeRunConfig>(config.approach_config)) {
    return "ridge";
  }
  return "discrete-grid";
}

std::string strategy_name(const BacktestRunConfig& config) {
  if (const auto* buy_and_hold = std::get_if<BuyAndHoldRunConfig>(&config.approach_config)) {
    if (buy_and_hold->strategy == BuyAndHoldStrategy::ScheduledBuy) {
      return "scheduled-buy";
    }
    return "lump-sum";
  }
  if (const auto* discrete_grid = std::get_if<DiscreteGridRunConfig>(&config.approach_config)) {
    if (discrete_grid->strategy == DiscreteGridStrategy::Gain) {
      return "gain";
    }
    if (discrete_grid->strategy == DiscreteGridStrategy::Regime) {
      return "regime";
    }
    return "drop";
  }
  const auto& ridge = std::get<RidgeRunConfig>(config.approach_config);
  if (ridge.strategy == RidgeStrategy::LongShort) {
    return "long-short";
  }
  if (ridge.strategy == RidgeStrategy::VolTarget) {
    return "vol-target";
  }
  return "directional";
}

}  // namespace metis
