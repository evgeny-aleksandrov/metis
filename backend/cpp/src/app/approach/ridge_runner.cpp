#include "metis/app/approach/ridge_runner.hpp"

#include <stdexcept>

namespace metis {

BacktestExecutionResult RidgeRunner::run(
    const BacktestRunConfig& /*config*/,
    const std::vector<Candle>& /*prices*/) const {
  throw std::runtime_error("The ridge approach is recognized by the CLI but execution is not implemented yet.");
}

}  // namespace metis
