#include "metis/config/run_config.hpp"

#include <variant>

namespace metis {

GridSearchConfig grid_search_config_from_discrete_grid_config(const DiscreteGridRunConfig& config) {
  GridSearchConfig grid;
  grid.x_min = config.search_space.threshold_pct.min;
  grid.x_max = config.search_space.threshold_pct.max;
  grid.x_step = config.search_space.threshold_pct.step;
  grid.y_min = config.search_space.lookback_days.min;
  grid.y_max = config.search_space.lookback_days.max;
  grid.y_step = config.search_space.lookback_days.step;
  grid.fast_lookback_min = config.search_space.fast_lookback_days.min;
  grid.fast_lookback_max = config.search_space.fast_lookback_days.max;
  grid.fast_lookback_step = config.search_space.fast_lookback_days.step;
  grid.short_y_min = config.search_space.short_lookback_days.min;
  grid.short_y_max = config.search_space.short_lookback_days.max;
  grid.short_y_step = config.search_space.short_lookback_days.step;
  grid.short_fast_lookback_min = config.search_space.short_fast_lookback_days.min;
  grid.short_fast_lookback_max = config.search_space.short_fast_lookback_days.max;
  grid.short_fast_lookback_step = config.search_space.short_fast_lookback_days.step;
  grid.hold_days_min = config.search_space.hold_days.min;
  grid.hold_days_max = config.search_space.hold_days.max;
  grid.hold_days_step = config.search_space.hold_days.step;
  grid.take_profit_min = config.search_space.take_profit_pct.min;
  grid.take_profit_max = config.search_space.take_profit_pct.max;
  grid.take_profit_step = config.search_space.take_profit_pct.step;
  grid.stop_loss_min = config.search_space.stop_loss_pct.min;
  grid.stop_loss_max = config.search_space.stop_loss_pct.max;
  grid.stop_loss_step = config.search_space.stop_loss_pct.step;
  grid.trailing_stop_min = config.search_space.trailing_stop_pct.min;
  grid.trailing_stop_max = config.search_space.trailing_stop_pct.max;
  grid.trailing_stop_step = config.search_space.trailing_stop_pct.step;
  grid.regime_weak_exit_min = config.search_space.regime_weak_exit.min;
  grid.regime_weak_exit_max = config.search_space.regime_weak_exit.max;
  grid.allow_short_min = config.search_space.allow_short.min;
  grid.allow_short_max = config.search_space.allow_short.max;
  grid.volatility_lookback_min = config.search_space.volatility_lookback_days.min;
  grid.volatility_lookback_max = config.search_space.volatility_lookback_days.max;
  grid.volatility_lookback_step = config.search_space.volatility_lookback_days.step;
  grid.target_volatility_min = config.search_space.target_volatility.min;
  grid.target_volatility_max = config.search_space.target_volatility.max;
  grid.target_volatility_step = config.search_space.target_volatility.step;
  grid.max_position_pct_min = config.search_space.max_position_pct.min;
  grid.max_position_pct_max = config.search_space.max_position_pct.max;
  grid.max_position_pct_step = config.search_space.max_position_pct.step;
  grid.grid_sample_pct = config.search.sample_pct;
  grid.grid_sample_seed = config.search.seed;
  grid.grid_random_samples = config.search.random_samples;
  return grid;
}

StrategyType strategy_type_from_discrete_grid_strategy(DiscreteGridStrategy strategy) {
  if (strategy == DiscreteGridStrategy::Gain) {
    return StrategyType::Gain;
  }
  if (strategy == DiscreteGridStrategy::Regime) {
    return StrategyType::Regime;
  }
  return StrategyType::Drop;
}

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
