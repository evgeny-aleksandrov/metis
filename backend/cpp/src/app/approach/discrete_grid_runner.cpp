#include "metis/app/approach/discrete_grid_runner.hpp"

#include "metis/backtest/simulation.hpp"
#include "metis/backtest/walk_forward.hpp"
#include "metis/optimization/discrete_grid_search.hpp"

#include <variant>

namespace metis {

BacktestExecutionResult DiscreteGridRunner::run(
    const BacktestRunConfig& config,
    const std::vector<Candle>& prices) const {
  BacktestExecutionResult result;
  result.benchmark = run_buy_and_hold(
      prices,
      config.execution.initial_equity,
      config.execution.costs,
      config.execution.annualization);

  const DiscreteGridRunConfig& discrete_grid_config = std::get<DiscreteGridRunConfig>(config.approach_config);
  DiscreteGridSearchOptimizer optimizer(discrete_grid_config);
  result.summary = run_walk_forward(prices, config, optimizer, result.walk_forward_periods);
  result.has_walk_forward = true;
  return result;
}

}  // namespace metis
