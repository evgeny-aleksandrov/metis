#include "metis/app/backtester_workflow.hpp"

#include "metis/app/approach/buy_and_hold_runner.hpp"
#include "metis/app/approach/discrete_grid_runner.hpp"
#include "metis/app/approach/ridge_runner.hpp"

#include <stdexcept>
#include <variant>

namespace metis {

BacktestExecutionResult BacktesterWorkflow::run(
    const BacktestRunConfig& config,
    const std::vector<Candle>& prices) const {
  if (std::holds_alternative<BuyAndHoldRunConfig>(config.approach_config)) {
    return BuyAndHoldRunner{}.run(config, prices);
  }
  if (std::holds_alternative<DiscreteGridRunConfig>(config.approach_config)) {
    return DiscreteGridRunner{}.run(config, prices);
  }
  if (std::holds_alternative<RidgeRunConfig>(config.approach_config)) {
    return RidgeRunner{}.run(config, prices);
  }
  throw std::runtime_error("Unsupported approach.");
}

}  // namespace metis
