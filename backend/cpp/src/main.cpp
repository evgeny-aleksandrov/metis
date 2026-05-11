#include "backtest.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// Anonymous namespace keeps these symbols local to this .cpp file.
namespace {
// Runtime config collected from CLI flags.
struct CliConfig {
  std::string csv_path = "data/SPY.csv";
  std::string output_path = "ui/public/latest.json";
  std::string symbol = "SPY";
  std::string mode = "grid";
  StrategyType strategy = StrategyType::Drop;
  double initial_equity = 10000.0;
  int walk_train_months = 24;
  int walk_test_months = 6;
  GridSearchConfig grid;
};

struct DateParts {
  int year = 0;
  int month = 0;
  int day = 0;
};

struct WalkForwardPeriod {
  std::string train_start;
  std::string train_end;
  std::string test_start;
  std::string test_end;
  SimulationResult training_best;
  SimulationResult test_result;
};

// Prints CLI help text.
void print_help() {
  std::cout << "Metis C++ Backtester\n\n";
  std::cout << "Usage:\n";
  std::cout << "  metis_backtester [options]\n\n";
  std::cout << "Options:\n";
  std::cout << "  --symbol <ticker>     Symbol label for the output (default: SPY)\n";
  std::cout << "  --mode <name>         Run mode: grid or walk-forward (default: grid)\n";
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
  std::cout << "  --walk-train-months <n>  Months optimized before each test window (default: 24)\n";
  std::cout << "  --walk-test-months <n>   Months traded after each optimization (default: 6)\n";
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
    } else if (key == "--mode") {
      config.mode = need_value(key);
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
    } else if (key == "--walk-train-months") {
      config.walk_train_months = std::stoi(need_value(key));
    } else if (key == "--walk-test-months") {
      config.walk_test_months = std::stoi(need_value(key));
    } else {
      throw std::runtime_error("Unknown option: " + key);
    }
  }

  if (config.mode != "grid" && config.mode != "walk-forward") {
    throw std::runtime_error("Mode must be 'grid' or 'walk-forward'.");
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
  if (config.walk_train_months <= 0 || config.walk_test_months <= 0) {
    throw std::runtime_error("Walk-forward train/test months must be positive integers.");
  }
  if (config.grid.x_min > config.grid.x_max || config.grid.y_min > config.grid.y_max ||
      config.grid.hold_days_min > config.grid.hold_days_max) {
    throw std::runtime_error("Grid min cannot exceed max.");
  }
  return config;
}

int days_in_month(int year, int month) {
  static const int days_by_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2) {
    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  return days_by_month[month - 1];
}

DateParts parse_date(const std::string& value) {
  if (value.size() < 10) {
    throw std::runtime_error("Expected date in YYYY-MM-DD format: " + value);
  }
  return {std::stoi(value.substr(0, 4)), std::stoi(value.substr(5, 2)), std::stoi(value.substr(8, 2))};
}

std::string format_date(const DateParts& date) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << date.year << "-"
      << std::setw(2) << std::setfill('0') << date.month << "-"
      << std::setw(2) << std::setfill('0') << date.day;
  return out.str();
}

std::string add_months(const std::string& value, int months) {
  DateParts date = parse_date(value);
  const int zero_based_month = (date.month - 1) + months;
  date.year += zero_based_month / 12;
  date.month = (zero_based_month % 12) + 1;
  if (date.month <= 0) {
    date.month += 12;
    date.year -= 1;
  }
  const int max_day = days_in_month(date.year, date.month);
  if (date.day > max_day) {
    date.day = max_day;
  }
  return format_date(date);
}

std::vector<Candle> slice_prices(const std::vector<Candle>& prices, const std::string& start, const std::string& end) {
  std::vector<Candle> slice;
  for (const Candle& candle : prices) {
    if (candle.date >= start && candle.date < end) {
      slice.push_back(candle);
    }
  }
  return slice;
}

double compute_total_return(double initial_equity, double final_equity) {
  return initial_equity > 0.0 ? (final_equity / initial_equity) - 1.0 : 0.0;
}

double compute_cagr_from_bars(double initial_equity, double final_equity, size_t bars) {
  if (initial_equity <= 0.0 || final_equity <= 0.0 || bars < 2) {
    return 0.0;
  }
  const double years = static_cast<double>(bars - 1) / 252.0;
  return years > 0.0 ? std::pow(final_equity / initial_equity, 1.0 / years) - 1.0 : 0.0;
}

double compute_max_drawdown_from_points(const std::vector<EquityPoint>& points) {
  if (points.empty()) {
    return 0.0;
  }
  double peak = points.front().equity;
  double max_drawdown = 0.0;
  for (const EquityPoint& point : points) {
    if (point.equity > peak) {
      peak = point.equity;
    }
    if (peak > 0.0) {
      max_drawdown = std::max(max_drawdown, (peak - point.equity) / peak);
    }
  }
  return max_drawdown;
}

std::vector<double> returns_from_points(const std::vector<EquityPoint>& points) {
  std::vector<double> returns;
  if (points.size() < 2) {
    return returns;
  }
  returns.reserve(points.size() - 1);
  for (size_t index = 1; index < points.size(); ++index) {
    const double previous = points[index - 1].equity;
    const double current = points[index].equity;
    returns.push_back(previous > 0.0 ? (current - previous) / previous : 0.0);
  }
  return returns;
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

double compute_sharpe_from_points(const std::vector<EquityPoint>& points) {
  const std::vector<double> returns = returns_from_points(points);
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
  return stddev > 0.0 ? (mean / stddev) * std::sqrt(252.0) : 0.0;
}

double compute_sortino_from_points(const std::vector<EquityPoint>& points) {
  const std::vector<double> returns = returns_from_points(points);
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
  return downside_deviation > 0.0 ? (mean / downside_deviation) * std::sqrt(252.0) : 0.0;
}

SimulationResult run_walk_forward(const std::vector<Candle>& prices, const CliConfig& config, std::vector<WalkForwardPeriod>& periods) {
  if (prices.empty()) {
    throw std::runtime_error("Cannot run walk-forward without prices.");
  }

  SimulationResult combined;
  combined.params.strategy = config.strategy;
  combined.final_equity = config.initial_equity;
  double current_equity = config.initial_equity;
  size_t test_bars = 0;

  std::string test_start = add_months(prices.front().date, config.walk_train_months);
  while (test_start < prices.back().date) {
    const std::string train_start = add_months(test_start, -config.walk_train_months);
    const std::string test_end = add_months(test_start, config.walk_test_months);
    const std::vector<Candle> training_prices = slice_prices(prices, train_start, test_start);
    const std::vector<Candle> test_prices = slice_prices(prices, test_start, test_end);

    if (training_prices.size() < 30 || test_prices.size() < 2) { //See if this check if good enough
      break;
    }

    std::vector<SimulationResult> train_results = run_grid_search(training_prices, config.grid, config.strategy, config.initial_equity);
    sort_results_by_cagr(train_results);
    const SimulationResult training_best = train_results.front();
    SimulationResult test_result = run_simulation(test_prices, training_best.params, current_equity);

    current_equity = test_result.final_equity;
    test_bars += test_result.equity_curve.size();
    combined.equity_curve.insert(combined.equity_curve.end(), test_result.equity_curve.begin(), test_result.equity_curve.end());
    combined.trades.insert(combined.trades.end(), test_result.trades.begin(), test_result.trades.end());
    combined.metrics.trades += test_result.metrics.trades;
    periods.push_back({training_prices.front().date, training_prices.back().date, test_prices.front().date, test_prices.back().date, training_best, test_result});

    test_start = test_end;
  }

  if (periods.empty()) {
    throw std::runtime_error("Not enough data for walk-forward. Need at least train months plus one test window.");
  }

  int wins = 0;
  for (const Trade& trade : combined.trades) {
    if (trade.pnl > 0.0) {
      wins += 1;
    }
  }
  combined.final_equity = current_equity;
  combined.metrics.total_return = compute_total_return(config.initial_equity, combined.final_equity);
  combined.metrics.cagr = compute_cagr_from_bars(config.initial_equity, combined.final_equity, test_bars);
  combined.metrics.max_drawdown = compute_max_drawdown_from_points(combined.equity_curve);
  combined.metrics.sharpe = compute_sharpe_from_points(combined.equity_curve);
  combined.metrics.sortino = compute_sortino_from_points(combined.equity_curve);
  combined.metrics.win_rate = combined.metrics.trades > 0 ? static_cast<double>(wins) / static_cast<double>(combined.metrics.trades) : 0.0;
  return combined;
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
  out << "  \"mode\": \"" << json_escape(config.mode) << "\",\n";
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

std::string to_walk_forward_json(const SimulationResult& combined, const std::vector<WalkForwardPeriod>& periods, size_t bars, const CliConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.symbol) << "\",\n";
  out << "  \"mode\": \"" << json_escape(config.mode) << "\",\n";
  out << "  \"strategy\": \"" << strategy_type_to_string(config.strategy) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.initial_equity << ",\n";
  out << "  \"walk_forward\": {\n";
  out << "    \"train_months\": " << config.walk_train_months << ",\n";
  out << "    \"test_months\": " << config.walk_test_months << ",\n";
  out << "    \"periods\": " << periods.size() << "\n";
  out << "  },\n";
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
  write_result_json(out, combined, "  ");
  out << ",\n";
  out << "  \"equity_curve\": [\n";
  for (size_t index = 0; index < combined.equity_curve.size(); ++index) {
    const EquityPoint& point = combined.equity_curve[index];
    out << "    {\"date\": \"" << json_escape(point.date) << "\", \"equity\": " << point.equity << "}";
    if (index + 1 < combined.equity_curve.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
  out << "  \"trades\": [\n";
  for (size_t index = 0; index < combined.trades.size(); ++index) {
    const Trade& trade = combined.trades[index];
    out << "    {\n";
    out << "      \"entry_date\": \"" << json_escape(trade.entry_date) << "\",\n";
    out << "      \"exit_date\": \"" << json_escape(trade.exit_date) << "\",\n";
    out << "      \"entry_price\": " << trade.entry_price << ",\n";
    out << "      \"exit_price\": " << trade.exit_price << ",\n";
    out << "      \"shares\": " << trade.shares << ",\n";
    out << "      \"pnl\": " << trade.pnl << ",\n";
    out << "      \"holding_days\": " << trade.holding_days << "\n";
    out << "    }";
    if (index + 1 < combined.trades.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
  out << "  \"walk_forward_periods\": [\n";
  for (size_t index = 0; index < periods.size(); ++index) {
    const WalkForwardPeriod& period = periods[index];
    out << "    {\n";
    out << "      \"train_start\": \"" << json_escape(period.train_start) << "\",\n";
    out << "      \"train_end\": \"" << json_escape(period.train_end) << "\",\n";
    out << "      \"test_start\": \"" << json_escape(period.test_start) << "\",\n";
    out << "      \"test_end\": \"" << json_escape(period.test_end) << "\",\n";
    out << "      \"training_best\": ";
    write_result_json(out, period.training_best, "      ");
    out << ",\n";
    out << "      \"test_result\": ";
    write_result_json(out, period.test_result, "      ");
    out << "\n";
    out << "    }";
    if (index + 1 < periods.size()) {
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
    std::string json;
    SimulationResult summary;
    size_t walk_periods = 0;

    if (config.mode == "walk-forward") {
      std::vector<WalkForwardPeriod> periods;
      summary = run_walk_forward(prices, config, periods);
      walk_periods = periods.size();
      json = to_walk_forward_json(summary, periods, prices.size(), config);
    } else {
      std::vector<SimulationResult> results = run_grid_search(prices, config.grid, config.strategy, config.initial_equity);

      if (results.empty()) {
        throw std::runtime_error("No backtest results were produced.");
      }

      sort_results_by_cagr(results);
      summary = results.front();
      json = to_json(results, summary, prices.size(), config);
    }

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
    std::cout << "Mode: " << config.mode << "\n";
    std::cout << "Strategy: " << strategy_type_to_string(config.strategy) << "\n";
    std::cout << "Rows loaded: " << prices.size() << "\n";
    if (config.mode == "walk-forward") {
      std::cout << "Walk-forward periods: " << walk_periods << "\n";
      std::cout << "Train months: " << config.walk_train_months << "\n";
      std::cout << "Test months: " << config.walk_test_months << "\n";
      std::cout << "Out-of-sample CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
    } else {
      std::cout << "Best threshold_pct: " << summary.params.diff_pct << "\n";
      std::cout << "Best lookback_days: " << summary.params.lookback_days << "\n";
      std::cout << "Best hold_days: " << summary.params.hold_days << "\n";
      std::cout << "Best CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
    }
    std::cout << "Output written: " << config.output_path << "\n";

    return 0;
  } catch (const std::exception& error) {
    // C++ exception handling is closest to Java try/catch.
    std::cerr << "Error: " << error.what() << "\n";
    return 1;
  }
}
