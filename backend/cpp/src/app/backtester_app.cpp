#include "metis/app/backtester_app.hpp"

#include "metis/backtest/simulation.hpp"
#include "metis/backtest/walk_forward.hpp"
#include "metis/backtest/metrics.hpp"
#include "metis/cli/approach.hpp"
#include "metis/cli/cli_config.hpp"
#include "metis/core/file_io.hpp"
#include "metis/data/csv_loader.hpp"
#include "metis/optimization/discrete_grid_search.hpp"
#include "metis/reporting/console_summary.hpp"
#include "metis/reporting/json_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace metis {

int BacktesterApp::run(int argc, char** argv) {
  try {
    const CliConfig options = parse_args(argc, argv);
    const BacktestRunConfig config = to_run_config(options);
    const std::vector<Candle> prices = load_prices_from_csv(config.data.csv_path);
    const SimulationResult buy_and_hold = run_buy_and_hold(
        prices,
        config.execution.initial_equity,
        config.execution.costs,
        config.execution.annualization);

    if (config.approach == ApproachType::BuyAndHold) {
      const std::string json = to_json({}, buy_and_hold, buy_and_hold, prices.size(), config);
      write_text_file(config.output.json_path, json, "backtest output");
      print_summary(config, prices.size(), buy_and_hold, buy_and_hold, 0);
      return 0;
    }

    DiscreteGridSearchOptimizer optimizer(config.discrete_search);
    std::vector<WalkForwardPeriod> periods;
    const SimulationResult summary = run_walk_forward(prices, config, optimizer, periods);
    const std::string json = to_walk_forward_json(summary, buy_and_hold, periods, prices.size(), config);

    write_text_file(config.output.json_path, json, "backtest output");
    print_summary(config, prices.size(), buy_and_hold, summary, periods.size());
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
}

}  // namespace metis
