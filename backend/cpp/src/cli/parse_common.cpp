#include "metis/cli/parse_common.hpp"

#include "metis/cli/approach.hpp"

#include <stdexcept>

namespace metis {

std::string OptionCursor::current() const {
  return argv[index];
}

std::string OptionCursor::need_value(const std::string& option) {
  if (index + 1 >= argc) {
    throw std::runtime_error("Missing value for option: " + option);
  }
  index += 1;
  return argv[index];
}

bool is_help_token(const std::string& value) {
  return value == "--help" || value == "-h";
}

bool parse_common_option(CliConfig& config, OptionCursor& cursor) {
  const std::string key = cursor.current();
  if (key == "--csv") {
    config.csv_path = cursor.need_value(key);
  } else if (key == "--output") {
    config.output_path = cursor.need_value(key);
  } else if (key == "--symbol") {
    config.symbol = cursor.need_value(key);
  } else if (key == "--initial") {
    config.initial_equity = std::stod(cursor.need_value(key));
  } else if (key == "--fixed-cost") {
    config.costs.fixed_per_order = std::stod(cursor.need_value(key));
  } else if (key == "--variable-cost-rate") {
    config.costs.variable_rate = std::stod(cursor.need_value(key));
  } else if (key == "--bars-per-year") {
    config.annualization.bars_per_year = std::stod(cursor.need_value(key));
  } else if (key == "--walk-train-months") {
    config.walk_train_months = std::stoi(cursor.need_value(key));
  } else if (key == "--walk-test-months") {
    config.walk_test_months = std::stoi(cursor.need_value(key));
  } else if (key == "--training-results-dir") {
    config.training_results_dir = cursor.need_value(key);
  } else {
    return false;
  }
  return true;
}

void validate_common_config(const CliConfig& config) {
  if (!is_strategy_supported_by_approach(config.approach, config.strategy)) {
    throw std::runtime_error(
        "Strategy '" + strategy_family_to_string(config.strategy) + "' is not valid for approach '" +
        approach_type_to_string(config.approach) + "'.");
  }
  if (config.walk_train_months <= 0 || config.walk_test_months <= 0) {
    throw std::runtime_error("Walk-forward train/test months must be positive integers.");
  }
  if (config.costs.fixed_per_order < 0.0 || config.costs.variable_rate < 0.0) {
    throw std::runtime_error("Transaction costs cannot be negative.");
  }
  if (config.annualization.bars_per_year <= 0.0) {
    throw std::runtime_error("Bars per year must be positive.");
  }
}

}  // namespace metis
