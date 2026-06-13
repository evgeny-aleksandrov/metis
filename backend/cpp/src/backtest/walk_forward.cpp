#include "metis/backtest/walk_forward.hpp"

#include "metis/backtest/metrics.hpp"
#include "metis/backtest/training_candidate_selector.hpp"
#include "metis/backtest/walk_forward_splitter.hpp"
#include "metis/core/date.hpp"
#include "metis/reporting/training_results_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace metis {
namespace {

void append_test_result(SimulationResult& combined, const SimulationResult& test_result) {
  combined.equity_curve.insert(combined.equity_curve.end(), test_result.equity_curve.begin(), test_result.equity_curve.end());
  combined.trades.insert(combined.trades.end(), test_result.trades.begin(), test_result.trades.end());
  combined.metrics.trades += test_result.metrics.trades;
}

}  // namespace

SimulationResult run_walk_forward(
    const std::vector<Candle>& prices,
    const BacktestRunConfig& config,
    const Optimizer& optimizer,
    const WalkForwardCandidateRunner& candidate_runner,
    std::vector<WalkForwardPeriod>& periods) {
  if (prices.empty()) {
    throw std::runtime_error("Cannot run walk-forward without prices.");
  }

  SimulationResult combined;
  combined.final_equity = config.execution.initial_equity;
  double current_equity = config.execution.initial_equity;
  size_t test_bars = 0;
  const std::string training_results_run_id = now_utc_compact();
  const std::filesystem::path training_results_dir = training_results_run_dir(config, training_results_run_id);
  const std::vector<WalkForwardWindow> windows = WalkForwardSplitter{}.split(prices, config.walk_forward);
  const int minimum_training_trades = std::max(1, config.walk_forward.train_months / 2);
  const TrainingCandidateSelector selector(minimum_training_trades, 0.80);

  for (const WalkForwardWindow& window : windows) {
    const std::vector<SimulationResult> train_results = optimizer.evaluate(window.training_prices, config.execution);
    const SelectedTrainingCandidate selected = selector.select(train_results, window);
    if (!config.output.training_results_dir.empty()) {
      write_training_results_csv(
          training_results_dir,
          periods.size() + 1,
          window.train_start,
          window.train_end,
          window.test_start,
          window.test_end,
          selected.eligible_results);
    }

    const StrategyParams selected_params = selected.result.params;
    combined.params = selected_params;
    const SimulationResult training_selection =
        candidate_runner.run_training_selection(window, selected_params, config.execution);
    const SimulationResult test_result =
        candidate_runner.run_test(window, selected_params, current_equity, config.execution);

    current_equity = test_result.final_equity;
    test_bars += test_result.equity_curve.size();
    append_test_result(combined, test_result);
    periods.push_back({
        window.train_start,
        window.train_end,
        window.test_start,
        window.test_end,
        training_selection,
        test_result});
  }

  if (periods.empty()) {
    throw std::runtime_error("Not enough data for walk-forward. Need at least train months plus one test window.");
  }

  int wins = 0;
  for (const Trade& trade : combined.trades) {
    if (trade.pnl > 0.0) {
      wins += 1;
    }
  }
  combined.final_equity = current_equity;
  combined.metrics.total_return = compute_total_return(config.execution.initial_equity, combined.final_equity);
  combined.metrics.cagr = compute_cagr_from_bars(config.execution.initial_equity, combined.final_equity, test_bars, config.execution.annualization.bars_per_year);
  combined.metrics.max_drawdown = compute_max_drawdown_from_points(combined.equity_curve);
  combined.metrics.sharpe = compute_sharpe_from_points(combined.equity_curve, config.execution.annualization.bars_per_year);
  combined.metrics.sortino = compute_sortino_from_points(combined.equity_curve, config.execution.annualization.bars_per_year);
  combined.metrics.win_rate = combined.metrics.trades > 0 ? static_cast<double>(wins) / static_cast<double>(combined.metrics.trades) : 0.0;
  return combined;
}

}  // namespace metis
