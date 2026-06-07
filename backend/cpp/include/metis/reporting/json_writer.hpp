#pragma once

#include "metis/backtest/walk_forward.hpp"
#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

std::string json_escape(const std::string& value);
std::string to_json(
    const std::vector<SimulationResult>& results,
    const SimulationResult& best,
    const SimulationResult& buy_and_hold,
    size_t bars,
    const BacktestRunConfig& config);
std::string to_walk_forward_json(
    const SimulationResult& combined,
    const SimulationResult& buy_and_hold,
    const std::vector<WalkForwardPeriod>& periods,
    size_t bars,
    const BacktestRunConfig& config);

}  // namespace metis
