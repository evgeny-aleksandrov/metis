#include "backtest.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

// Anonymous namespace keeps these symbols local to this .cpp file.
namespace {
// Runtime config collected from CLI flags.
struct CliConfig {
  std::string csv_path = "data/SPY.csv";
  std::string output_path = "ui/public/latest.json";
  std::string symbol = "SPY";
  StrategyType strategy = StrategyType::Drop;
  double initial_equity = 10000.0;
  GridSearchConfig grid;
};

// Prints CLI help text.
void print_help() {
  std::cout << "Metis C++ Backtester\n\n";
  std::cout << "Usage:\n";
  std::cout << "  metis_backtester [options]\n\n";
  std::cout << "Options:\n";
  std::cout << "  --symbol <ticker>     Symbol label for the output (default: SPY)\n";
  std::cout << "  --strategy <name>     Strategy type: drop or gain (default: drop)\n";
  std::cout << "  --csv <path>          CSV file with Date + Close/Adj Close columns\n";
  std::cout << "  --output <path>       JSON output path (default: ui/public/latest.json)\n";
  std::cout << "  --initial <amount>    Starting cash (default: 10000)\n";
  std::cout << "  --x-min <value>       Min diff percent decimal (default: -0.10)\n";
  std::cout << "  --x-max <value>       Max diff percent decimal (default: 0.10)\n";
  std::cout << "  --x-step <value>      Diff sweep step (default: 0.01)\n";
  std::cout << "  --y-min <days>        Min lookback days (default: 2)\n";
  std::cout << "  --y-max <days>        Max lookback days (default: 20)\n";
  std::cout << "  --y-step <days>       Lookback sweep step (default: 1)\n";
  std::cout << "  --hold-min <days>     Min hold days after buy signal (default: 10)\n";
  std::cout << "  --hold-max <days>     Max hold days after buy signal (default: 60)\n";
  std::cout << "  --hold-step <days>    Hold-day sweep step (default: 5)\n";
  std::cout << "  --help                Show this help\n";
}

// Minimal manual argument parser.
// Similar idea to parsing `sys.argv` in Python or `args[]` in Java main().
CliConfig parse_args(int argc, char** argv) {
  CliConfig config;
  for (int index = 1; index < argc; ++index) {
    const std::string key = argv[index];
    // Lambda (inline function object). Captures `index`/`argc`/`argv` from outer scope.
    auto need_value = [&](const std::string& option) -> std::string {
      if (index + 1 >= argc) {
        throw std::runtime_error("Missing value for option: " + option);
      }
      index += 1;
      return argv[index];
    };

    if (key == "--help") {
      print_help();
      std::exit(0);
    } else if (key == "--csv") {
      config.csv_path = need_value(key);
    } else if (key == "--output") {
      config.output_path = need_value(key);
    } else if (key == "--symbol") {
      config.symbol = need_value(key);
    } else if (key == "--strategy") {
      config.strategy = strategy_type_from_string(need_value(key));
    } else if (key == "--initial") {
      config.initial_equity = std::stod(need_value(key));
    } else if (key == "--x-min") {
      config.grid.x_min = std::stod(need_value(key));
    } else if (key == "--x-max") {
      config.grid.x_max = std::stod(need_value(key));
    } else if (key == "--x-step") {
      config.grid.x_step = std::stod(need_value(key));
    } else if (key == "--y-min") {
      config.grid.y_min = std::stoi(need_value(key));
    } else if (key == "--y-max") {
      config.grid.y_max = std::stoi(need_value(key));
    } else if (key == "--y-step") {
      config.grid.y_step = std::stoi(need_value(key));
    } else if (key == "--hold-min") {
      config.grid.hold_days_min = std::stoi(need_value(key));
    } else if (key == "--hold-max") {
      config.grid.hold_days_max = std::stoi(need_value(key));
    } else if (key == "--hold-step") {
      config.grid.hold_days_step = std::stoi(need_value(key));
    } else {
      throw std::runtime_error("Unknown option: " + key);
    }
  }

  if (config.grid.x_step <= 0.0) {
    throw std::runtime_error("X grid values must be positive.");
  }
  if (config.grid.x_min < 0.0 || config.grid.x_max < 0.0) {
    throw std::runtime_error("X threshold values must be non-negative.");
  }
  if (config.grid.y_min <= 0 || config.grid.y_max <= 0 || config.grid.y_step <= 0 ||
      config.grid.hold_days_min <= 0 || config.grid.hold_days_max <= 0 || config.grid.hold_days_step <= 0) {
    throw std::runtime_error("Y/hold values must be positive integers.");
  }
  if (config.grid.x_min > config.grid.x_max || config.grid.y_min > config.grid.y_max ||
      config.grid.hold_days_min > config.grid.hold_days_max) {
    throw std::runtime_error("Grid min cannot exceed max.");
  }
  return config;
}

// Builds an ISO-8601 UTC timestamp string.
std::string now_utc_iso8601() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm{};
#ifdef _WIN32
  gmtime_s(&utc_tm, &now_time);
#else
  gmtime_r(&now_time, &utc_tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string json_escape(const std::string& value) {
  std::ostringstream out;
  for (char ch : value) {
    if (ch == '"' || ch == '\\') {
      out << '\\' << ch;
    } else if (ch == '\n') {
      out << "\\n";
    } else if (ch == '\r') {
      out << "\\r";
    } else if (ch == '\t') {
      out << "\\t";
    } else {
      out << ch;
    }
  }
  return out.str();
}

void write_result_json(std::ostringstream& out, const SimulationResult& item, const std::string& indent) {
  out << indent << "{\n";
  out << indent << "  \"strategy\": \"" << strategy_type_to_string(item.params.strategy) << "\",\n";
  out << indent << "  \"diff_pct\": " << item.params.diff_pct << ",\n";
  out << indent << "  \"lookback_days\": " << item.params.lookback_days << ",\n";
  out << indent << "  \"hold_days\": " << item.params.hold_days << ",\n";
  out << indent << "  \"final_equity\": " << item.final_equity << ",\n";
  out << indent << "  \"total_return\": " << item.metrics.total_return << ",\n";
  out << indent << "  \"cagr\": " << item.metrics.cagr << ",\n";
  out << indent << "  \"max_drawdown\": " << item.metrics.max_drawdown << ",\n";
  out << indent << "  \"sharpe\": " << item.metrics.sharpe << ",\n";
  out << indent << "  \"sortino\": " << item.metrics.sortino << ",\n";
  out << indent << "  \"trades\": " << item.metrics.trades << ",\n";
  out << indent << "  \"win_rate\": " << item.metrics.win_rate << "\n";
  out << indent << "}";
}

// Hand-written JSON serializer for result output.
// C++ streams (`<<`) are similar to building strings with StringBuilder in Java.
std::string to_json(const std::vector<SimulationResult>& results, const SimulationResult& best, size_t bars, const CliConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.symbol) << "\",\n";
  out << "  \"strategy\": \"" << strategy_type_to_string(config.strategy) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.initial_equity << ",\n";
  out << "  \"threshold_pct\": {\n";
  out << "    \"min\": " << config.grid.x_min << ",\n";
  out << "    \"max\": " << config.grid.x_max << ",\n";
  out << "    \"step\": " << config.grid.x_step << "\n";
  out << "  },\n";
  out << "  \"lookback_days\": {\n";
  out << "    \"min\": " << config.grid.y_min << ",\n";
  out << "    \"max\": " << config.grid.y_max << ",\n";
  out << "    \"step\": " << config.grid.y_step << "\n";
  out << "  },\n";
  out << "  \"hold_days\": {\n";
  out << "    \"min\": " << config.grid.hold_days_min << ",\n";
  out << "    \"max\": " << config.grid.hold_days_max << ",\n";
  out << "    \"step\": " << config.grid.hold_days_step << "\n";
  out << "  },\n";
  out << "  \"best\": ";
  write_result_json(out, best, "  ");
  out << ",\n";
  out << "  \"equity_curve\": [\n";
  for (size_t index = 0; index < best.equity_curve.size(); ++index) {
    const EquityPoint& point = best.equity_curve[index];
    out << "    {\"date\": \"" << json_escape(point.date) << "\", \"equity\": " << point.equity << "}";
    if (index + 1 < best.equity_curve.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
  out << "  \"trades\": [\n";
  for (size_t index = 0; index < best.trades.size(); ++index) {
    const Trade& trade = best.trades[index];
    out << "    {\n";
    out << "      \"entry_date\": \"" << json_escape(trade.entry_date) << "\",\n";
    out << "      \"exit_date\": \"" << json_escape(trade.exit_date) << "\",\n";
    out << "      \"entry_price\": " << trade.entry_price << ",\n";
    out << "      \"exit_price\": " << trade.exit_price << ",\n";
    out << "      \"shares\": " << trade.shares << ",\n";
    out << "      \"pnl\": " << trade.pnl << ",\n";
    out << "      \"holding_days\": " << trade.holding_days << "\n";
    out << "    }";
    if (index + 1 < best.trades.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
  out << "  \"results\": [\n";

  for (size_t index = 0; index < results.size(); ++index) {
    const SimulationResult& item = results[index];
    write_result_json(out, item, "    ");
    if (index + 1 < results.size()) {
      out << ",";
    }
    out << "\n";
  }

  out << "  ]\n";
  out << "}\n";
  return out.str();
}
}  // namespace

// Program entry point (equivalent to Java `public static void main`).
int main(int argc, char** argv) {
  try {
    const CliConfig config = parse_args(argc, argv);
    const std::vector<Candle> prices = load_prices_from_csv(config.csv_path);
    std::vector<SimulationResult> results = run_grid_search(prices, config.grid, config.strategy, config.initial_equity);

    if (results.empty()) {
      throw std::runtime_error("No backtest results were produced.");
    }

    // Sort descending by CAGR, then by final equity as tie-breaker.
    // Lambda comparator is like passing a custom comparator in Java/Python sort.
    std::sort(results.begin(), results.end(), [](const SimulationResult& left, const SimulationResult& right) {
      if (left.metrics.cagr == right.metrics.cagr) {
        return left.final_equity > right.final_equity;
      }
      return left.metrics.cagr > right.metrics.cagr;
    });

    const SimulationResult best = results.front();
    const std::string json = to_json(results, best, prices.size(), config);

    const std::filesystem::path output_path(config.output_path);
    if (!output_path.parent_path().empty()) {
      std::filesystem::create_directories(output_path.parent_path());
    }
    std::ofstream out_file(output_path);
    if (!out_file.is_open()) {
      throw std::runtime_error("Could not write output: " + config.output_path);
    }
    out_file << json;
    out_file.close();

    std::cout << "Backtest complete.\n";
    std::cout << "Symbol: " << config.symbol << "\n";
    std::cout << "Strategy: " << strategy_type_to_string(config.strategy) << "\n";
    std::cout << "Rows loaded: " << prices.size() << "\n";
    std::cout << "Best threshold_pct: " << best.params.diff_pct << "\n";
    std::cout << "Best lookback_days: " << best.params.lookback_days << "\n";
    std::cout << "Best hold_days: " << best.params.hold_days << "\n";
    std::cout << "Best CAGR: " << std::fixed << std::setprecision(4) << (best.metrics.cagr * 100.0) << "%\n";
    std::cout << "Output written: " << config.output_path << "\n";

    return 0;
  } catch (const std::exception& error) {
    // C++ exception handling is closest to Java try/catch.
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
}
