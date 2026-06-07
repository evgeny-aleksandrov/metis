#include "metis/backtest/walk_forward.hpp"

#include "metis/backtest/metrics.hpp"
#include "metis/backtest/optimization.hpp"
#include "metis/backtest/simulation.hpp"
#include "metis/core/date.hpp"
#include "metis/reporting/training_results_writer.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <stdexcept>

namespace metis {
namespace {

std::vector<Candle> slice_prices(const std::vector<Candle>& prices, const std::string& start, const std::string& end) {
  std::vector<Candle> slice;
  for (const Candle& candle : prices) {
    if (candle.date >= start && candle.date < end) {
      slice.push_back(candle);
    }
  }
  return slice;
}

}  // namespace

SimulationResult run_walk_forward(
    const std::vector<Candle>& prices,
    const CliConfig& config,
    std::vector<WalkForwardPeriod>& periods) {
  if (prices.empty()) {
    throw std::runtime_error("Cannot run walk-forward without prices.");
  }

  SimulationResult combined;
  combined.params.strategy = config.strategy;
  combined.final_equity = config.initial_equity;
  double current_equity = config.initial_equity;
  size_t test_bars = 0;
  const std::string training_results_run_id = now_utc_compact();
  const std::filesystem::path training_results_dir = training_results_run_dir(config, training_results_run_id);

  std::string test_start = add_months(prices.front().date, config.walk_train_months);
  while (test_start < prices.back().date) {
    const std::string train_start = add_months(test_start, -config.walk_train_months);
    const std::string test_end = add_months(test_start, config.walk_test_months);
    if (test_end > prices.back().date) {
      break;
    }
    const std::vector<Candle> training_prices = slice_prices(prices, train_start, test_start);
    const std::vector<Candle> test_prices = slice_prices(prices, test_start, test_end);

    if (training_prices.size() < 30 || test_prices.size() < 30) {
      break;
    }

    std::vector<SimulationResult> train_results =
        run_grid_search(training_prices, config.grid, config.strategy, config.initial_equity, config.costs, config.annualization);
    if (train_results.empty()) {
      throw std::runtime_error("No training results were produced. Check that fast lookback values are below slow lookback values.");
    }
    const int minimum_training_trades = std::max(1, config.walk_train_months / 2);
    std::vector<SimulationResult> eligible_train_results = filter_training_candidates(train_results, minimum_training_trades, 0.80);
    if (eligible_train_results.empty()) {
      std::ostringstream message;
      message << "No eligible training results for "
              << training_prices.front().date << ".." << training_prices.back().date
              << ". Required at least " << minimum_training_trades
              << " trades and win_rate >= 0.80.";
      throw std::runtime_error(message.str());
    }
    sort_results_by_sortino(eligible_train_results);
    if (!config.training_results_dir.empty()) {
      write_training_results_csv(
          training_results_dir,
          periods.size() + 1,
          training_prices.front().date,
          training_prices.back().date,
          test_prices.front().date,
          test_prices.back().date,
          eligible_train_results);
    }
    const StrategyParams selected_params = eligible_train_results.front().params;
    const SimulationResult training_selection =
        run_simulation(training_prices, selected_params, config.initial_equity, config.costs, config.annualization);
    SimulationResult test_result = run_simulation(test_prices, selected_params, current_equity, config.costs, config.annualization);

    current_equity = test_result.final_equity;
    test_bars += test_result.equity_curve.size();
    combined.equity_curve.insert(combined.equity_curve.end(), test_result.equity_curve.begin(), test_result.equity_curve.end());
    combined.trades.insert(combined.trades.end(), test_result.trades.begin(), test_result.trades.end());
    combined.metrics.trades += test_result.metrics.trades;
    periods.push_back({training_prices.front().date, training_prices.back().date, test_prices.front().date, test_prices.back().date, training_selection, test_result});

    test_start = test_end;
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
  combined.metrics.total_return = compute_total_return(config.initial_equity, combined.final_equity);
  combined.metrics.cagr = compute_cagr_from_bars(config.initial_equity, combined.final_equity, test_bars, config.annualization.bars_per_year);
  combined.metrics.max_drawdown = compute_max_drawdown_from_points(combined.equity_curve);
  combined.metrics.sharpe = compute_sharpe_from_points(combined.equity_curve, config.annualization.bars_per_year);
  combined.metrics.sortino = compute_sortino_from_points(combined.equity_curve, config.annualization.bars_per_year);
  combined.metrics.win_rate = combined.metrics.trades > 0 ? static_cast<double>(wins) / static_cast<double>(combined.metrics.trades) : 0.0;
  return combined;
}

}  // namespace metis
