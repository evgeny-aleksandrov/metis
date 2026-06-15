#include "metis/app/approach/discrete_grid_runner.hpp"

#include "metis/backtest/simulation.hpp"
#include "metis/backtest/simulation_rules.hpp"
#include "metis/backtest/walk_forward.hpp"
#include "metis/optimization/discrete_grid_search.hpp"
#include "metis/strategy/strategy.hpp"

#include <variant>

namespace metis {
namespace {

class DiscreteGridWalkForwardCandidateRunner final : public WalkForwardCandidateRunner {
public:
  SimulationResult run_training_selection(
      const WalkForwardWindow& window,
      const StrategyParams& selected_params,
      const ExecutionConfig& execution) const override {
    const DiscreteGridStrategyParams params = std::get<DiscreteGridStrategyParams>(selected_params);
    const DiscreteStrategy selected_strategy(params);
    const SimulationRules rules = simulation_rules_from(params, selected_strategy);
    return run_simulation(window.training_prices, selected_strategy, rules, execution);
  }

  SimulationResult run_test(
      const WalkForwardWindow& window,
      const StrategyParams& selected_params,
      double current_equity,
      const ExecutionConfig& execution) const override {
    const DiscreteGridStrategyParams params = std::get<DiscreteGridStrategyParams>(selected_params);
    const DiscreteStrategy selected_strategy(params);
    const SimulationRules rules = simulation_rules_from(params, selected_strategy);
    return run_simulation(
        window.test_prices,
        selected_strategy,
        rules,
        current_equity,
        execution.costs,
        execution.annualization);
  }
};

}  // namespace

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
  const DiscreteGridWalkForwardCandidateRunner candidate_runner;
  result.summary = run_walk_forward(prices, config, optimizer, candidate_runner, result.walk_forward_periods);
  result.has_walk_forward = true;
  return result;
}

}  // namespace metis
