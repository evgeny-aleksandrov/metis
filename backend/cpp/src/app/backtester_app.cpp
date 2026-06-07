#include "metis/app/backtester_app.hpp"

#include "metis/app/backtester_workflow.hpp"
#include "metis/cli/cli_config.hpp"
#include "metis/core/file_io.hpp"
#include "metis/data/csv_loader.hpp"
#include "metis/reporting/console_summary.hpp"
#include "metis/reporting/json_writer.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace metis {

int BacktesterApp::run(int argc, char** argv) {
  try {
    const CliConfig options = parse_args(argc, argv);
    const BacktestRunConfig config = to_run_config(options);
    const std::vector<Candle> prices = load_prices_from_csv(config.data.csv_path);
    const BacktestExecutionResult result = BacktesterWorkflow{}.run(config, prices);
    const std::string json = to_backtest_report_json(config, prices.size(), result);

    write_text_file(config.output.json_path, json, "backtest output");
    print_summary(
        config,
        prices.size(),
        result.benchmark,
        result.summary,
        result.walk_forward_periods.size());
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
}

}  // namespace metis
