#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <vector>

namespace metis {

double compute_total_return(double initial_equity, double final_equity);
double compute_cagr_from_bars(double initial_equity, double final_equity, size_t bars, double bars_per_year);
double compute_max_drawdown(const std::vector<double>& equity_curve);
double compute_max_drawdown_from_points(const std::vector<EquityPoint>& points);
std::vector<double> returns_from_values(const std::vector<double>& values);
std::vector<double> returns_from_points(const std::vector<EquityPoint>& points);
double mean_return(const std::vector<double>& returns);
double compute_sharpe(const std::vector<double>& equity_curve, double bars_per_year);
double compute_sharpe_from_points(const std::vector<EquityPoint>& points, double bars_per_year);
double compute_sortino(const std::vector<double>& equity_curve, double bars_per_year);
double compute_sortino_from_points(const std::vector<EquityPoint>& points, double bars_per_year);
void sort_results_by_sortino(std::vector<SimulationResult>& results);
std::vector<SimulationResult> filter_training_candidates(
    const std::vector<SimulationResult>& results,
    int minimum_trades,
    double minimum_win_rate);

}  // namespace metis
