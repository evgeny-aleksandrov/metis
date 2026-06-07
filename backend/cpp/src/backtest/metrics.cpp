#include "metis/backtest/metrics.hpp"

#include <algorithm>
#include <cmath>

namespace metis {

double compute_total_return(double initial_equity, double final_equity) {
  return initial_equity > 0.0 ? (final_equity / initial_equity) - 1.0 : 0.0;
}

double compute_cagr_from_bars(double initial_equity, double final_equity, size_t bars, double bars_per_year) {
  if (initial_equity <= 0.0 || final_equity <= 0.0 || bars < 2) {
    return 0.0;
  }
  const double years = static_cast<double>(bars - 1) / bars_per_year;
  return years > 0.0 ? std::pow(final_equity / initial_equity, 1.0 / years) - 1.0 : 0.0;
}

double compute_max_drawdown(const std::vector<double>& equity_curve) {
  if (equity_curve.empty()) {
    return 0.0;
  }
  double peak = equity_curve.front();
  double max_drawdown = 0.0;
  for (double value : equity_curve) {
    if (value > peak) {
      peak = value;
    }
    if (peak > 0.0) {
      max_drawdown = std::max(max_drawdown, (peak - value) / peak);
    }
  }
  return max_drawdown;
}

double compute_max_drawdown_from_points(const std::vector<EquityPoint>& points) {
  std::vector<double> values;
  values.reserve(points.size());
  for (const EquityPoint& point : points) {
    values.push_back(point.equity);
  }
  return compute_max_drawdown(values);
}

std::vector<double> returns_from_values(const std::vector<double>& values) {
  std::vector<double> returns;
  if (values.size() < 2) {
    return returns;
  }
  returns.reserve(values.size() - 1);
  for (size_t index = 1; index < values.size(); ++index) {
    const double previous = values[index - 1];
    const double current = values[index];
    returns.push_back(previous > 0.0 ? (current - previous) / previous : 0.0);
  }
  return returns;
}

std::vector<double> returns_from_points(const std::vector<EquityPoint>& points) {
  std::vector<double> values;
  values.reserve(points.size());
  for (const EquityPoint& point : points) {
    values.push_back(point.equity);
  }
  return returns_from_values(values);
}

double mean_return(const std::vector<double>& returns) {
  if (returns.empty()) {
    return 0.0;
  }
  double mean = 0.0;
  for (double value : returns) {
    mean += value;
  }
  return mean / static_cast<double>(returns.size());
}

double compute_sharpe(const std::vector<double>& equity_curve, double bars_per_year) {
  const std::vector<double> returns = returns_from_values(equity_curve);
  if (returns.empty()) {
    return 0.0;
  }
  const double mean = mean_return(returns);
  double variance = 0.0;
  for (double value : returns) {
    const double diff = value - mean;
    variance += diff * diff;
  }
  variance /= static_cast<double>(returns.size());
  const double stddev = std::sqrt(variance);
  return stddev > 0.0 ? (mean / stddev) * std::sqrt(bars_per_year) : 0.0;
}

double compute_sharpe_from_points(const std::vector<EquityPoint>& points, double bars_per_year) {
  std::vector<double> values;
  values.reserve(points.size());
  for (const EquityPoint& point : points) {
    values.push_back(point.equity);
  }
  return compute_sharpe(values, bars_per_year);
}

double compute_sortino(const std::vector<double>& equity_curve, double bars_per_year) {
  const std::vector<double> returns = returns_from_values(equity_curve);
  if (returns.empty()) {
    return 0.0;
  }
  const double mean = mean_return(returns);
  double downside_variance = 0.0;
  size_t downside_count = 0;
  for (double value : returns) {
    if (value < 0.0) {
      downside_variance += value * value;
      downside_count += 1;
    }
  }
  if (downside_count == 0) {
    return 0.0;
  }
  const double downside_deviation = std::sqrt(downside_variance / static_cast<double>(downside_count));
  return downside_deviation > 0.0 ? (mean / downside_deviation) * std::sqrt(bars_per_year) : 0.0;
}

double compute_sortino_from_points(const std::vector<EquityPoint>& points, double bars_per_year) {
  std::vector<double> values;
  values.reserve(points.size());
  for (const EquityPoint& point : points) {
    values.push_back(point.equity);
  }
  return compute_sortino(values, bars_per_year);
}

void sort_results_by_sortino(std::vector<SimulationResult>& results) {
  std::sort(results.begin(), results.end(), [](const SimulationResult& left, const SimulationResult& right) {
    if (left.metrics.sortino == right.metrics.sortino) {
      return left.metrics.cagr > right.metrics.cagr;
    }
    return left.metrics.sortino > right.metrics.sortino;
  });
}

std::vector<SimulationResult> filter_training_candidates(
    const std::vector<SimulationResult>& results,
    int minimum_trades,
    double minimum_win_rate) {
  std::vector<SimulationResult> filtered;
  for (const SimulationResult& result : results) {
    if (result.metrics.trades >= minimum_trades && result.metrics.win_rate >= minimum_win_rate) {
      filtered.push_back(result);
    }
  }
  return filtered;
}

}  // namespace metis
