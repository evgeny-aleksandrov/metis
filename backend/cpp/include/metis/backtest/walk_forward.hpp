#pragma once

#include "metis/config/run_config.hpp"
#include "metis/optimization/optimizer.hpp"
#include "metis/backtest/walk_forward_splitter.hpp"
#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

struct WalkForwardPeriod {
  std::string train_start;
  std::string train_end;
  std::string test_start;
  std::string test_end;
  SimulationResult training_selection;
  SimulationResult test_result;
};

class WalkForwardCandidateRunner {
public:
  virtual ~WalkForwardCandidateRunner() = default;

  virtual SimulationResult run_training_selection(
      const WalkForwardWindow& window,
      const StrategyParams& selected_params,
      const ExecutionConfig& execution) const = 0;

  virtual SimulationResult run_test(
      const WalkForwardWindow& window,
      const StrategyParams& selected_params,
      double current_equity,
      const ExecutionConfig& execution) const = 0;
};

SimulationResult run_walk_forward(
    const std::vector<Candle>& prices,
    const BacktestRunConfig& config,
    const Optimizer& optimizer,
    const WalkForwardCandidateRunner& candidate_runner,
    std::vector<WalkForwardPeriod>& periods);

}  // namespace metis
