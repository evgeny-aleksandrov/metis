#include "metis/app/approach/buy_and_hold_runner.hpp"

#include "metis/backtest/simulation.hpp"

namespace metis {

BacktestExecutionResult BuyAndHoldRunner::run(
    const BacktestRunConfig& config,
    const std::vector<Candle>& prices) const {
  BacktestExecutionResult result;
  result.benchmark = run_buy_and_hold(
      prices,
      config.execution.initial_equity,
      config.execution.costs,
      config.execution.annualization);
  result.summary = result.benchmark;
  result.has_walk_forward = false;
  return result;
}

}  // namespace metis
