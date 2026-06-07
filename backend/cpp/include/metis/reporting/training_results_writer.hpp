#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace metis {

std::string sanitize_path_part(const std::string& value);
std::string csv_escape(const std::string& value);
std::filesystem::path training_results_run_dir(const BacktestRunConfig& config, const std::string& run_id);
void write_training_results_csv(
    const std::filesystem::path& run_dir,
    size_t period_index,
    const std::string& train_start,
    const std::string& train_end,
    const std::string& test_start,
    const std::string& test_end,
    const std::vector<SimulationResult>& train_results);

}  // namespace metis
