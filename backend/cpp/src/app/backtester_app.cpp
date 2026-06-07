#include "metis/app/backtester_app.hpp"

#include "metis/backtest/optimization.hpp"
#include "metis/backtest/simulation.hpp"
#include "metis/backtest/walk_forward.hpp"
#include "metis/backtest/metrics.hpp"
#include "metis/cli/cli_config.hpp"
#include "metis/core/file_io.hpp"
#include "metis/data/csv_loader.hpp"
#include "metis/reporting/console_summary.hpp"
#include "metis/reporting/json_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace metis {

int BacktesterApp::run(int argc, char** argv) {
  try {
    const CliConfig config = parse_args(argc, argv);
    const std::vector<Candle> prices = load_prices_from_csv(config.csv_path);
    const SimulationResult buy_and_hold = run_buy_and_hold(prices, config.initial_equity, config.costs, config.annualization);
    std::string json;
    SimulationResult summary;
    size_t walk_periods = 0;

    if (config.mode == "walk-forward") {
      std::vector<WalkForwardPeriod> periods;
      summary = run_walk_forward(prices, config, periods);
      walk_periods = periods.size();
      json = to_walk_forward_json(summary, buy_and_hold, periods, prices.size(), config);
    } else {
      std::vector<SimulationResult> results =
          run_grid_search(prices, config.grid, config.strategy, config.initial_equity, config.costs, config.annualization);
      if (results.empty()) {
        throw std::runtime_error("No backtest results were produced. Check that fast lookback values are below slow lookback values.");
      }
      sort_results_by_sortino(results);
      summary = results.front();
      json = to_json(results, summary, buy_and_hold, prices.size(), config);
    }

    write_text_file(config.output_path, json, "backtest output");
    print_summary(config, prices.size(), buy_and_hold, summary, walk_periods);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
}

}  // namespace metis
