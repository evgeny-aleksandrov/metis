#include "metis/backtest/training_candidate_selector.hpp"

#include "metis/backtest/metrics.hpp"

#include <sstream>
#include <stdexcept>

namespace metis {

TrainingCandidateSelector::TrainingCandidateSelector(int minimum_trades, double minimum_win_rate)
    : minimum_trades_(minimum_trades), minimum_win_rate_(minimum_win_rate) {}

SelectedTrainingCandidate TrainingCandidateSelector::select(
    const std::vector<SimulationResult>& train_results,
    const WalkForwardWindow& window) const {
  if (train_results.empty()) {
    throw std::runtime_error("No training results were produced. Check that fast lookback values are below slow lookback values.");
  }

  std::vector<SimulationResult> eligible_results =
      filter_training_candidates(train_results, minimum_trades_, minimum_win_rate_);
  if (eligible_results.empty()) {
    std::ostringstream message;
    message << "No eligible training results for "
            << window.train_start << ".." << window.train_end
            << ". Required at least " << minimum_trades_
            << " trades and win_rate >= " << minimum_win_rate_ << ".";
    throw std::runtime_error(message.str());
  }

  sort_results_by_sortino(eligible_results);
  return {eligible_results, eligible_results.front()};
}

}  // namespace metis
