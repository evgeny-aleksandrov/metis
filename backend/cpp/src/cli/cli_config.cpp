#include "metis/cli/cli_config.hpp"

#include "metis/cli/approach.hpp"
#include "metis/cli/approach/buy_and_hold_cli.hpp"
#include "metis/cli/approach/discrete_grid_cli.hpp"
#include "metis/cli/approach/ridge_cli.hpp"
#include "metis/cli/help.hpp"
#include "metis/cli/parse_common.hpp"

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace metis {
namespace {

bool parse_approach_option(CliConfig& config, OptionCursor& cursor) {
  if (config.approach == ApproachType::BuyAndHold) {
    return parse_buy_and_hold_option(config, cursor);
  }
  if (config.approach == ApproachType::DiscreteGrid) {
    return parse_discrete_grid_option(config, cursor);
  }
  if (config.approach == ApproachType::Ridge) {
    return parse_ridge_option(config, cursor);
  }
  return false;
}

void validate_approach_config(const CliConfig& config) {
  if (config.approach == ApproachType::BuyAndHold) {
    validate_buy_and_hold_config(config);
  } else if (config.approach == ApproachType::DiscreteGrid) {
    validate_discrete_grid_config(config);
  } else if (config.approach == ApproachType::Ridge) {
    validate_ridge_config(config);
  }
}

}  // namespace

void print_help() {
  print_general_help();
}

CliConfig parse_args(int argc, char** argv) {
  if (argc <= 1) {
    print_general_help();
    std::exit(1);
  }

  const std::string first = argv[1];
  if (is_help_token(first)) {
    print_general_help();
    std::exit(0);
  }

  CliConfig config;
  config.approach = approach_type_from_string(first);

  if (argc <= 2) {
    print_approach_help(config.approach);
    std::exit(1);
  }

  const std::string second = argv[2];
  if (is_help_token(second)) {
    print_approach_help(config.approach);
    std::exit(0);
  }

  config.strategy = strategy_family_from_string(config.approach, second);

  OptionCursor cursor{argc, argv, 3};
  while (cursor.index < argc) {
    if (is_help_token(cursor.current())) {
      print_strategy_help(config.approach, config.strategy);
      std::exit(0);
    }

    if (!parse_common_option(config, cursor) && !parse_approach_option(config, cursor)) {
      throw std::runtime_error("Unknown option: " + cursor.current());
    }
    cursor.index += 1;
  }

  validate_common_config(config);
  validate_approach_config(config);
  return config;
}

BacktestRunConfig to_run_config(const CliConfig& options) {
  BacktestRunConfig config;
  config.approach = options.approach;
  config.strategy = options.strategy;
  config.data.csv_path = options.csv_path;
  config.data.symbol = options.symbol;
  config.output.json_path = options.output_path;
  config.output.training_results_dir = options.training_results_dir;
  config.execution.initial_equity = options.initial_equity;
  config.execution.costs = options.costs;
  config.execution.annualization = options.annualization;
  config.walk_forward.train_months = options.walk_train_months;
  config.walk_forward.test_months = options.walk_test_months;
  if (options.approach == ApproachType::DiscreteGrid) {
    config.discrete_search.strategy = discrete_strategy_type_from_family(options.strategy);
  }
  config.discrete_search.grid = options.grid;
  return config;
}

}  // namespace metis
