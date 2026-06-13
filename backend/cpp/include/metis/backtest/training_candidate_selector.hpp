#pragma once

#include "metis/backtest/walk_forward_splitter.hpp"
#include "metis/types.hpp"

#include <vector>

namespace metis {

struct SelectedTrainingCandidate {
  std::vector<SimulationResult> eligible_results;
  SimulationResult result;
};

class TrainingCandidateSelector {
public:
  explicit TrainingCandidateSelector(int minimum_trades, double minimum_win_rate);

  SelectedTrainingCandidate select(
      const std::vector<SimulationResult>& train_results,
      const WalkForwardWindow& window) const;

private:
  int minimum_trades_;
  double minimum_win_rate_;
};

}  // namespace metis
