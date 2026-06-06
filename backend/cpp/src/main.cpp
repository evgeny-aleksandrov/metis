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
  TransactionCosts costs;
  Annualization annualization;
  int walk_train_months = 24;
  int walk_test_months = 6;
  std::string training_results_dir = "outputs/backtests/training-results";
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
  SimulationResult training_selection;
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
  std::cout << "  --strategy <name>     Strategy type: drop, gain, or regime (default: drop)\n";
  std::cout << "  --csv <path>          CSV file with Date + Close/Adj Close columns\n";
  std::cout << "  --output <path>       JSON output path (default: ui/public/latest.json)\n";
  std::cout << "  --initial <amount>    Starting cash (default: 10000)\n";
  std::cout << "  --fixed-cost <amount> Fixed transaction cost per order (default: 0)\n";
  std::cout << "  --variable-cost-rate <rate> Variable transaction cost rate per order (default: 0)\n";
  std::cout << "  --bars-per-year <n>  Bars per year for CAGR/Sharpe annualization (default: 252)\n";
  std::cout << "  --x-min <value>       Min diff percent decimal (default: -0.10)\n";
  std::cout << "  --x-max <value>       Max diff percent decimal (default: 0.10)\n";
  std::cout << "  --x-step <value>      Diff sweep step (default: 0.01)\n";
  std::cout << "  --y-min <days>        Min lookback days (default: 2)\n";
  std::cout << "  --y-max <days>        Max lookback days (default: 20)\n";
  std::cout << "  --y-step <days>       Lookback sweep step (default: 1)\n";
  std::cout << "  --fast-lookback-min <days>  Min fast MA lookback for regime crossover (default: 2)\n";
  std::cout << "  --fast-lookback-max <days>  Max fast MA lookback for regime crossover (default: 10)\n";
  std::cout << "  --fast-lookback-step <days> Fast MA lookback sweep step (default: 1)\n";
  std::cout << "  --short-y-min <days>        Min short slow MA lookback; 0 mirrors long slow MA (default: 0)\n";
  std::cout << "  --short-y-max <days>        Max short slow MA lookback; 0 mirrors long slow MA (default: 0)\n";
  std::cout << "  --short-y-step <days>       Short slow MA sweep step (default: 1)\n";
  std::cout << "  --short-fast-lookback-min <days> Min short fast MA lookback; 0 mirrors long fast MA (default: 0)\n";
  std::cout << "  --short-fast-lookback-max <days> Max short fast MA lookback; 0 mirrors long fast MA (default: 0)\n";
  std::cout << "  --short-fast-lookback-step <days> Short fast MA sweep step (default: 1)\n";
  std::cout << "  --hold-min <days>     Min hold days after buy signal (default: 10)\n";
  std::cout << "  --hold-max <days>     Max hold days after buy signal (default: 60)\n";
  std::cout << "  --hold-step <days>    Hold-day sweep step (default: 5)\n";
  std::cout << "  --take-profit-min <value>     Min take-profit percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --take-profit-max <value>     Max take-profit percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --take-profit-step <value>    Take-profit sweep step (default: 0.01)\n";
  std::cout << "  --stop-loss-min <value>       Min stop-loss percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --stop-loss-max <value>       Max stop-loss percent decimal; 0 disables it (default: 0)\n";
  std::cout << "  --stop-loss-step <value>      Stop-loss sweep step (default: 0.01)\n";
  std::cout << "  --trailing-stop-min <value>   Min trailing stop after take-profit trigger; 0 makes TP immediate (default: 0)\n";
  std::cout << "  --trailing-stop-max <value>   Max trailing stop after take-profit trigger (default: 0)\n";
  std::cout << "  --trailing-stop-step <value>  Trailing stop sweep step (default: 0.01)\n";
  std::cout << "  --regime-weak-exit-mode <mode> Regime weakness exit mode: off, on, or both (default: off)\n";
  std::cout << "  --short-mode <mode>     Short selling mode: off, on, or both (default: off)\n";
  std::cout << "  --vol-lookback-min <bars> Min realized-vol lookback; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --vol-lookback-max <bars> Max realized-vol lookback; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --vol-lookback-step <bars> Realized-vol lookback sweep step (default: 1)\n";
  std::cout << "  --target-vol-min <value> Min annualized target vol; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --target-vol-max <value> Max annualized target vol; 0 disables vol sizing (default: 0)\n";
  std::cout << "  --target-vol-step <value> Target-vol sweep step (default: 0.01)\n";
  std::cout << "  --max-position-pct-min <value> Min equity fraction used as notional cap (default: 1)\n";
  std::cout << "  --max-position-pct-max <value> Max equity fraction used as notional cap (default: 1)\n";
  std::cout << "  --max-position-pct-step <value> Position cap sweep step (default: 0.25)\n";
  std::cout << "  --grid-sample-pct <value> Random fraction of full grid to evaluate, 0-1 (default: 1)\n";
  std::cout << "  --grid-sample-seed <n> Deterministic random seed for sampled grids (default: 42)\n";
  std::cout << "  --grid-random-samples <n> Draw n random regime parameter sets directly; 0 uses grid loops (default: 0)\n";
  std::cout << "  --walk-train-months <n>  Months optimized before each test window (default: 24)\n";
  std::cout << "  --walk-test-months <n>   Months traded after each optimization (default: 6)\n";
  std::cout << "  Walk-forward selection filters for trades >= 50% of train months and win rate >= 80%, then picks best Sortino.\n";
  std::cout << "  --training-results-dir <path> Directory for per-period training result CSVs; empty disables export\n";
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
    } else if (key == "--fixed-cost") {
      config.costs.fixed_per_order = std::stod(need_value(key));
    } else if (key == "--variable-cost-rate") {
      config.costs.variable_rate = std::stod(need_value(key));
    } else if (key == "--bars-per-year") {
      config.annualization.bars_per_year = std::stod(need_value(key));
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
    } else if (key == "--fast-lookback-min") {
      config.grid.fast_lookback_min = std::stoi(need_value(key));
    } else if (key == "--fast-lookback-max") {
      config.grid.fast_lookback_max = std::stoi(need_value(key));
    } else if (key == "--fast-lookback-step") {
      config.grid.fast_lookback_step = std::stoi(need_value(key));
    } else if (key == "--short-y-min") {
      config.grid.short_y_min = std::stoi(need_value(key));
    } else if (key == "--short-y-max") {
      config.grid.short_y_max = std::stoi(need_value(key));
    } else if (key == "--short-y-step") {
      config.grid.short_y_step = std::stoi(need_value(key));
    } else if (key == "--short-fast-lookback-min") {
      config.grid.short_fast_lookback_min = std::stoi(need_value(key));
    } else if (key == "--short-fast-lookback-max") {
      config.grid.short_fast_lookback_max = std::stoi(need_value(key));
    } else if (key == "--short-fast-lookback-step") {
      config.grid.short_fast_lookback_step = std::stoi(need_value(key));
    } else if (key == "--hold-min") {
      config.grid.hold_days_min = std::stoi(need_value(key));
    } else if (key == "--hold-max") {
      config.grid.hold_days_max = std::stoi(need_value(key));
    } else if (key == "--hold-step") {
      config.grid.hold_days_step = std::stoi(need_value(key));
    } else if (key == "--take-profit-min") {
      config.grid.take_profit_min = std::stod(need_value(key));
    } else if (key == "--take-profit-max") {
      config.grid.take_profit_max = std::stod(need_value(key));
    } else if (key == "--take-profit-step") {
      config.grid.take_profit_step = std::stod(need_value(key));
    } else if (key == "--stop-loss-min") {
      config.grid.stop_loss_min = std::stod(need_value(key));
    } else if (key == "--stop-loss-max") {
      config.grid.stop_loss_max = std::stod(need_value(key));
    } else if (key == "--stop-loss-step") {
      config.grid.stop_loss_step = std::stod(need_value(key));
    } else if (key == "--trailing-stop-min") {
      config.grid.trailing_stop_min = std::stod(need_value(key));
    } else if (key == "--trailing-stop-max") {
      config.grid.trailing_stop_max = std::stod(need_value(key));
    } else if (key == "--trailing-stop-step") {
      config.grid.trailing_stop_step = std::stod(need_value(key));
    } else if (key == "--regime-weak-exit-mode") {
      const std::string mode = need_value(key);
      if (mode == "off") {
        config.grid.regime_weak_exit_min = 0;
        config.grid.regime_weak_exit_max = 0;
      } else if (mode == "on") {
        config.grid.regime_weak_exit_min = 1;
        config.grid.regime_weak_exit_max = 1;
      } else if (mode == "both") {
        config.grid.regime_weak_exit_min = 0;
        config.grid.regime_weak_exit_max = 1;
      } else {
        throw std::runtime_error("Regime weak exit mode must be off, on, or both.");
      }
    } else if (key == "--short-mode") {
      const std::string mode = need_value(key);
      if (mode == "off") {
        config.grid.allow_short_min = 0;
        config.grid.allow_short_max = 0;
      } else if (mode == "on") {
        config.grid.allow_short_min = 1;
        config.grid.allow_short_max = 1;
      } else if (mode == "both") {
        config.grid.allow_short_min = 0;
        config.grid.allow_short_max = 1;
      } else {
        throw std::runtime_error("Short mode must be off, on, or both.");
      }
    } else if (key == "--vol-lookback-min") {
      config.grid.volatility_lookback_min = std::stoi(need_value(key));
    } else if (key == "--vol-lookback-max") {
      config.grid.volatility_lookback_max = std::stoi(need_value(key));
    } else if (key == "--vol-lookback-step") {
      config.grid.volatility_lookback_step = std::stoi(need_value(key));
    } else if (key == "--target-vol-min") {
      config.grid.target_volatility_min = std::stod(need_value(key));
    } else if (key == "--target-vol-max") {
      config.grid.target_volatility_max = std::stod(need_value(key));
    } else if (key == "--target-vol-step") {
      config.grid.target_volatility_step = std::stod(need_value(key));
    } else if (key == "--max-position-pct-min") {
      config.grid.max_position_pct_min = std::stod(need_value(key));
    } else if (key == "--max-position-pct-max") {
      config.grid.max_position_pct_max = std::stod(need_value(key));
    } else if (key == "--max-position-pct-step") {
      config.grid.max_position_pct_step = std::stod(need_value(key));
    } else if (key == "--grid-sample-pct") {
      config.grid.grid_sample_pct = std::stod(need_value(key));
    } else if (key == "--grid-sample-seed") {
      config.grid.grid_sample_seed = static_cast<unsigned int>(std::stoul(need_value(key)));
    } else if (key == "--grid-random-samples") {
      config.grid.grid_random_samples = std::stoi(need_value(key));
    } else if (key == "--walk-train-months") {
      config.walk_train_months = std::stoi(need_value(key));
    } else if (key == "--walk-test-months") {
      config.walk_test_months = std::stoi(need_value(key));
    } else if (key == "--training-results-dir") {
      config.training_results_dir = need_value(key);
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
      config.grid.fast_lookback_min <= 0 || config.grid.fast_lookback_max <= 0 || config.grid.fast_lookback_step <= 0 ||
      config.grid.short_y_min < 0 || config.grid.short_y_max < 0 || config.grid.short_y_step <= 0 ||
      config.grid.short_fast_lookback_min < 0 || config.grid.short_fast_lookback_max < 0 || config.grid.short_fast_lookback_step <= 0 ||
      config.grid.hold_days_min <= 0 || config.grid.hold_days_max <= 0 || config.grid.hold_days_step <= 0 ||
      config.grid.volatility_lookback_min < 0 || config.grid.volatility_lookback_max < 0 || config.grid.volatility_lookback_step <= 0) {
    throw std::runtime_error("Lookback/hold values must be positive integers.");
  }
  if (config.grid.take_profit_step <= 0.0 || config.grid.stop_loss_step <= 0.0 || config.grid.trailing_stop_step <= 0.0 ||
      config.grid.target_volatility_step <= 0.0 || config.grid.max_position_pct_step <= 0.0 ||
      config.grid.take_profit_min < 0.0 || config.grid.take_profit_max < 0.0 ||
      config.grid.stop_loss_min < 0.0 || config.grid.stop_loss_max < 0.0 ||
      config.grid.trailing_stop_min < 0.0 || config.grid.trailing_stop_max < 0.0 ||
      config.grid.target_volatility_min < 0.0 || config.grid.target_volatility_max < 0.0 ||
      config.grid.max_position_pct_min < 0.0 || config.grid.max_position_pct_max < 0.0) {
    throw std::runtime_error("Risk sizing values must be non-negative and steps must be positive.");
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
  if (config.grid.grid_sample_pct <= 0.0 || config.grid.grid_sample_pct > 1.0) {
    throw std::runtime_error("Grid sample percent must be > 0 and <= 1.");
  }
  if (config.grid.grid_random_samples < 0) {
    throw std::runtime_error("Grid random samples cannot be negative.");
  }
  if (config.grid.x_min > config.grid.x_max || config.grid.y_min > config.grid.y_max ||
      config.grid.fast_lookback_min > config.grid.fast_lookback_max ||
      config.grid.short_y_min > config.grid.short_y_max ||
      config.grid.short_fast_lookback_min > config.grid.short_fast_lookback_max ||
      config.grid.hold_days_min > config.grid.hold_days_max ||
      config.grid.take_profit_min > config.grid.take_profit_max ||
      config.grid.stop_loss_min > config.grid.stop_loss_max ||
      config.grid.trailing_stop_min > config.grid.trailing_stop_max ||
      config.grid.regime_weak_exit_min > config.grid.regime_weak_exit_max ||
      config.grid.allow_short_min > config.grid.allow_short_max ||
      config.grid.volatility_lookback_min > config.grid.volatility_lookback_max ||
      config.grid.target_volatility_min > config.grid.target_volatility_max ||
      config.grid.max_position_pct_min > config.grid.max_position_pct_max) {
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

double compute_cagr_from_bars(double initial_equity, double final_equity, size_t bars, double bars_per_year) {
  if (initial_equity <= 0.0 || final_equity <= 0.0 || bars < 2) {
    return 0.0;
  }
  const double years = static_cast<double>(bars - 1) / bars_per_year;
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

double compute_sharpe_from_points(const std::vector<EquityPoint>& points, double bars_per_year) {
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
  return stddev > 0.0 ? (mean / stddev) * std::sqrt(bars_per_year) : 0.0;
}

double compute_sortino_from_points(const std::vector<EquityPoint>& points, double bars_per_year) {
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
  return downside_deviation > 0.0 ? (mean / downside_deviation) * std::sqrt(bars_per_year) : 0.0;
}

void sort_results_by_sortino(std::vector<SimulationResult>& results) {
  std::sort(results.begin(), results.end(), [](const SimulationResult& left, const SimulationResult& right) {
    if (left.metrics.sortino == right.metrics.sortino) {
      return left.metrics.cagr > right.metrics.cagr;
    }
    return left.metrics.sortino > right.metrics.sortino;
  });
}

std::vector<SimulationResult> filter_training_candidates(
    const std::vector<SimulationResult>& results,
    int minimum_trades,
    double minimum_win_rate) {
  std::vector<SimulationResult> filtered;
  for (const SimulationResult& result : results) {
    if (result.metrics.trades >= minimum_trades && result.metrics.win_rate >= minimum_win_rate) {
      filtered.push_back(result);
    }
  }
  return filtered;
}

std::string now_utc_compact();
std::filesystem::path training_results_run_dir(const CliConfig& config, const std::string& run_id);
void write_training_results_csv(
    const std::filesystem::path& run_dir,
    size_t period_index,
    const std::string& train_start,
    const std::string& train_end,
    const std::string& test_start,
    const std::string& test_end,
    const std::vector<SimulationResult>& train_results);

SimulationResult run_walk_forward(const std::vector<Candle>& prices, const CliConfig& config, std::vector<WalkForwardPeriod>& periods) {
  if (prices.empty()) {
    throw std::runtime_error("Cannot run walk-forward without prices.");
  }

  SimulationResult combined;
  combined.params.strategy = config.strategy;
  combined.final_equity = config.initial_equity;
  double current_equity = config.initial_equity;
  size_t test_bars = 0;
  const std::string training_results_run_id = now_utc_compact();
  const std::filesystem::path training_results_dir = training_results_run_dir(config, training_results_run_id);

  std::string test_start = add_months(prices.front().date, config.walk_train_months);
  while (test_start < prices.back().date) {
    const std::string train_start = add_months(test_start, -config.walk_train_months);
    const std::string test_end = add_months(test_start, config.walk_test_months);
    if (test_end > prices.back().date) {
      break;
    }
    const std::vector<Candle> training_prices = slice_prices(prices, train_start, test_start);
    const std::vector<Candle> test_prices = slice_prices(prices, test_start, test_end);

    if (training_prices.size() < 30 || test_prices.size() < 30) {
      break;
    }

    std::vector<SimulationResult> train_results =
        run_grid_search(training_prices, config.grid, config.strategy, config.initial_equity, config.costs, config.annualization);
    if (train_results.empty()) {
      throw std::runtime_error("No training results were produced. Check that fast lookback values are below slow lookback values.");
    }
    const int minimum_training_trades = std::max(1, config.walk_train_months / 2);
    std::vector<SimulationResult> eligible_train_results = filter_training_candidates(train_results, minimum_training_trades, 0.80);
    if (eligible_train_results.empty()) {
      std::ostringstream message;
      message << "No eligible training results for "
              << training_prices.front().date << ".." << training_prices.back().date
              << ". Required at least " << minimum_training_trades
              << " trades and win_rate >= 0.80.";
      throw std::runtime_error(message.str());
    }
    sort_results_by_sortino(eligible_train_results);
    if (!config.training_results_dir.empty()) {
      write_training_results_csv(
          training_results_dir,
          periods.size() + 1,
          training_prices.front().date,
          training_prices.back().date,
          test_prices.front().date,
          test_prices.back().date,
          eligible_train_results);
    }
    const StrategyParams selected_params = eligible_train_results.front().params;
    const SimulationResult training_selection =
        run_simulation(training_prices, selected_params, config.initial_equity, config.costs, config.annualization);
    SimulationResult test_result = run_simulation(test_prices, selected_params, current_equity, config.costs, config.annualization);

    current_equity = test_result.final_equity;
    test_bars += test_result.equity_curve.size();
    combined.equity_curve.insert(combined.equity_curve.end(), test_result.equity_curve.begin(), test_result.equity_curve.end());
    combined.trades.insert(combined.trades.end(), test_result.trades.begin(), test_result.trades.end());
    combined.metrics.trades += test_result.metrics.trades;
    periods.push_back({training_prices.front().date, training_prices.back().date, test_prices.front().date, test_prices.back().date, training_selection, test_result});

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
  combined.metrics.cagr = compute_cagr_from_bars(config.initial_equity, combined.final_equity, test_bars, config.annualization.bars_per_year);
  combined.metrics.max_drawdown = compute_max_drawdown_from_points(combined.equity_curve);
  combined.metrics.sharpe = compute_sharpe_from_points(combined.equity_curve, config.annualization.bars_per_year);
  combined.metrics.sortino = compute_sortino_from_points(combined.equity_curve, config.annualization.bars_per_year);
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

std::string now_utc_compact() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm{};
#ifdef _WIN32
  gmtime_s(&utc_tm, &now_time);
#else
  gmtime_r(&now_time, &utc_tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&utc_tm, "%Y%m%d-%H%M%SZ");
  return oss.str();
}

std::string sanitize_path_part(const std::string& value) {
  std::string sanitized;
  sanitized.reserve(value.size());
  for (char ch : value) {
    const bool keep = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' || ch == '_';
    sanitized.push_back(keep ? ch : '-');
  }
  return sanitized;
}

std::string csv_escape(const std::string& value) {
  bool needs_quotes = false;
  for (char ch : value) {
    if (ch == ',' || ch == '"' || ch == '\n' || ch == '\r') {
      needs_quotes = true;
      break;
    }
  }
  if (!needs_quotes) {
    return value;
  }
  std::string escaped = "\"";
  for (char ch : value) {
    if (ch == '"') {
      escaped += "\"\"";
    } else {
      escaped += ch;
    }
  }
  escaped += "\"";
  return escaped;
}

std::filesystem::path training_results_run_dir(const CliConfig& config, const std::string& run_id) {
  std::ostringstream name;
  name << run_id
       << "_" << sanitize_path_part(config.symbol)
       << "_" << strategy_type_to_string(config.strategy)
       << "_train" << config.walk_train_months << "m"
       << "_test" << config.walk_test_months << "m"
       << "_seed" << config.grid.grid_sample_seed
       << "_samples" << config.grid.grid_random_samples
       << "_mintrades50pct_winrate80_sortinobest";
  return std::filesystem::path(config.training_results_dir) / name.str();
}

void write_training_results_csv(
    const std::filesystem::path& run_dir,
    size_t period_index,
    const std::string& train_start,
    const std::string& train_end,
    const std::string& test_start,
    const std::string& test_end,
    const std::vector<SimulationResult>& train_results) {
  std::filesystem::create_directories(run_dir);
  std::ostringstream filename;
  filename << "period_" << std::setw(3) << std::setfill('0') << period_index
           << "_train_" << sanitize_path_part(train_start)
           << "_to_" << sanitize_path_part(train_end)
           << "_test_" << sanitize_path_part(test_start)
           << "_to_" << sanitize_path_part(test_end)
           << ".csv";
  const std::filesystem::path output_path = run_dir / filename.str();
  std::ofstream out(output_path);
  if (!out.is_open()) {
    throw std::runtime_error("Could not write training results: " + output_path.string());
  }
  out << std::fixed << std::setprecision(6);
  out << "rank,train_start,train_end,test_start,test_end,strategy,diff_pct,lookback_days,fast_lookback_days,"
      << "short_lookback_days,short_fast_lookback_days,hold_days,take_profit_pct,stop_loss_pct,trailing_stop_pct,"
      << "exit_on_regime_weakness,allow_short,volatility_lookback_days,target_volatility,max_position_pct,"
      << "final_equity,total_return,cagr,max_drawdown,sharpe,sortino,trades,win_rate\n";
  for (size_t index = 0; index < train_results.size(); ++index) {
    const SimulationResult& item = train_results[index];
    out << (index + 1) << ","
        << csv_escape(train_start) << ","
        << csv_escape(train_end) << ","
        << csv_escape(test_start) << ","
        << csv_escape(test_end) << ","
        << csv_escape(strategy_type_to_string(item.params.strategy)) << ","
        << item.params.diff_pct << ","
        << item.params.lookback_days << ","
        << item.params.fast_lookback_days << ","
        << item.params.short_lookback_days << ","
        << item.params.short_fast_lookback_days << ","
        << item.params.hold_days << ","
        << item.params.take_profit_pct << ","
        << item.params.stop_loss_pct << ","
        << item.params.trailing_stop_pct << ","
        << (item.params.exit_on_regime_weakness ? 1 : 0) << ","
        << (item.params.allow_short ? 1 : 0) << ","
        << item.params.volatility_lookback_days << ","
        << item.params.target_volatility << ","
        << item.params.max_position_pct << ","
        << item.final_equity << ","
        << item.metrics.total_return << ","
        << item.metrics.cagr << ","
        << item.metrics.max_drawdown << ","
        << item.metrics.sharpe << ","
        << item.metrics.sortino << ","
        << item.metrics.trades << ","
        << item.metrics.win_rate << "\n";
  }
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
  out << indent << "  \"fast_lookback_days\": " << item.params.fast_lookback_days << ",\n";
  out << indent << "  \"short_lookback_days\": " << item.params.short_lookback_days << ",\n";
  out << indent << "  \"short_fast_lookback_days\": " << item.params.short_fast_lookback_days << ",\n";
  out << indent << "  \"hold_days\": " << item.params.hold_days << ",\n";
  out << indent << "  \"take_profit_pct\": " << item.params.take_profit_pct << ",\n";
  out << indent << "  \"stop_loss_pct\": " << item.params.stop_loss_pct << ",\n";
  out << indent << "  \"trailing_stop_pct\": " << item.params.trailing_stop_pct << ",\n";
  out << indent << "  \"exit_on_regime_weakness\": " << (item.params.exit_on_regime_weakness ? "true" : "false") << ",\n";
  out << indent << "  \"allow_short\": " << (item.params.allow_short ? "true" : "false") << ",\n";
  out << indent << "  \"volatility_lookback_days\": " << item.params.volatility_lookback_days << ",\n";
  out << indent << "  \"target_volatility\": " << item.params.target_volatility << ",\n";
  out << indent << "  \"max_position_pct\": " << item.params.max_position_pct << ",\n";
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

void write_benchmark_json(std::ostringstream& out, const SimulationResult& item, const std::string& indent) {
  out << indent << "{\n";
  out << indent << "  \"final_equity\": " << item.final_equity << ",\n";
  out << indent << "  \"total_return\": " << item.metrics.total_return << ",\n";
  out << indent << "  \"cagr\": " << item.metrics.cagr << ",\n";
  out << indent << "  \"max_drawdown\": " << item.metrics.max_drawdown << ",\n";
  out << indent << "  \"sharpe\": " << item.metrics.sharpe << ",\n";
  out << indent << "  \"sortino\": " << item.metrics.sortino << ",\n";
  out << indent << "  \"trades\": " << item.metrics.trades << ",\n";
  out << indent << "  \"costs\": " << (item.trades.empty() ? 0.0 : item.trades.front().costs) << "\n";
  out << indent << "}";
}

// Hand-written JSON serializer for result output.
// C++ streams (`<<`) are similar to building strings with StringBuilder in Java.
std::string to_json(const std::vector<SimulationResult>& results, const SimulationResult& best, const SimulationResult& buy_and_hold, size_t bars, const CliConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.symbol) << "\",\n";
  out << "  \"mode\": \"" << json_escape(config.mode) << "\",\n";
  out << "  \"strategy\": \"" << strategy_type_to_string(config.strategy) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.initial_equity << ",\n";
  out << "  \"bars_per_year\": " << config.annualization.bars_per_year << ",\n";
  out << "  \"transaction_costs\": {\n";
  out << "    \"fixed_per_order\": " << config.costs.fixed_per_order << ",\n";
  out << "    \"variable_rate\": " << config.costs.variable_rate << "\n";
  out << "  },\n";
  out << "  \"selection\": {\n";
  out << "    \"mode\": \"filtered-best-sortino\",\n";
  out << "    \"rank_by\": \"sortino\",\n";
  out << "    \"minimum_training_trades\": " << std::max(1, config.walk_train_months / 2) << ",\n";
  out << "    \"minimum_training_win_rate\": 0.800000,\n";
  out << "    \"top_k\": 1\n";
  out << "  },\n";
  out << "  \"grid_sampling\": {\n";
  out << "    \"sample_pct\": " << config.grid.grid_sample_pct << ",\n";
  out << "    \"sample_seed\": " << config.grid.grid_sample_seed << ",\n";
  out << "    \"random_samples\": " << config.grid.grid_random_samples << "\n";
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
  out << "  \"regime\": {\n";
  out << "    \"fast_lookback_min\": " << config.grid.fast_lookback_min << ",\n";
  out << "    \"fast_lookback_max\": " << config.grid.fast_lookback_max << ",\n";
  out << "    \"fast_lookback_step\": " << config.grid.fast_lookback_step << ",\n";
  out << "    \"short_y_min\": " << config.grid.short_y_min << ",\n";
  out << "    \"short_y_max\": " << config.grid.short_y_max << ",\n";
  out << "    \"short_y_step\": " << config.grid.short_y_step << ",\n";
  out << "    \"short_fast_lookback_min\": " << config.grid.short_fast_lookback_min << ",\n";
  out << "    \"short_fast_lookback_max\": " << config.grid.short_fast_lookback_max << ",\n";
  out << "    \"short_fast_lookback_step\": " << config.grid.short_fast_lookback_step << ",\n";
  out << "    \"allow_short_min\": " << config.grid.allow_short_min << ",\n";
  out << "    \"allow_short_max\": " << config.grid.allow_short_max << "\n";
  out << "  },\n";
  out << "  \"risk_exits\": {\n";
  out << "    \"take_profit_min\": " << config.grid.take_profit_min << ",\n";
  out << "    \"take_profit_max\": " << config.grid.take_profit_max << ",\n";
  out << "    \"take_profit_step\": " << config.grid.take_profit_step << ",\n";
  out << "    \"stop_loss_min\": " << config.grid.stop_loss_min << ",\n";
  out << "    \"stop_loss_max\": " << config.grid.stop_loss_max << ",\n";
  out << "    \"stop_loss_step\": " << config.grid.stop_loss_step << ",\n";
  out << "    \"trailing_stop_min\": " << config.grid.trailing_stop_min << ",\n";
  out << "    \"trailing_stop_max\": " << config.grid.trailing_stop_max << ",\n";
  out << "    \"trailing_stop_step\": " << config.grid.trailing_stop_step << ",\n";
  out << "    \"regime_weak_exit_min\": " << config.grid.regime_weak_exit_min << ",\n";
  out << "    \"regime_weak_exit_max\": " << config.grid.regime_weak_exit_max << ",\n";
  out << "    \"volatility_lookback_min\": " << config.grid.volatility_lookback_min << ",\n";
  out << "    \"volatility_lookback_max\": " << config.grid.volatility_lookback_max << ",\n";
  out << "    \"volatility_lookback_step\": " << config.grid.volatility_lookback_step << ",\n";
  out << "    \"target_volatility_min\": " << config.grid.target_volatility_min << ",\n";
  out << "    \"target_volatility_max\": " << config.grid.target_volatility_max << ",\n";
  out << "    \"target_volatility_step\": " << config.grid.target_volatility_step << ",\n";
  out << "    \"max_position_pct_min\": " << config.grid.max_position_pct_min << ",\n";
  out << "    \"max_position_pct_max\": " << config.grid.max_position_pct_max << ",\n";
  out << "    \"max_position_pct_step\": " << config.grid.max_position_pct_step << "\n";
  out << "  },\n";
  out << "  \"buy_and_hold\": ";
  write_benchmark_json(out, buy_and_hold, "  ");
  out << ",\n";
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
    out << "      \"direction\": \"" << json_escape(trade.direction) << "\",\n";
    out << "      \"entry_price\": " << trade.entry_price << ",\n";
    out << "      \"exit_price\": " << trade.exit_price << ",\n";
    out << "      \"shares\": " << trade.shares << ",\n";
    out << "      \"pnl\": " << trade.pnl << ",\n";
    out << "      \"costs\": " << trade.costs << ",\n";
    out << "      \"entry_signal_quality\": " << trade.entry_signal_quality << ",\n";
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

std::string to_walk_forward_json(const SimulationResult& combined, const SimulationResult& buy_and_hold, const std::vector<WalkForwardPeriod>& periods, size_t bars, const CliConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.symbol) << "\",\n";
  out << "  \"mode\": \"" << json_escape(config.mode) << "\",\n";
  out << "  \"strategy\": \"" << strategy_type_to_string(config.strategy) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.initial_equity << ",\n";
  out << "  \"bars_per_year\": " << config.annualization.bars_per_year << ",\n";
  out << "  \"transaction_costs\": {\n";
  out << "    \"fixed_per_order\": " << config.costs.fixed_per_order << ",\n";
  out << "    \"variable_rate\": " << config.costs.variable_rate << "\n";
  out << "  },\n";
  out << "  \"selection\": {\n";
  out << "    \"mode\": \"filtered-best-sortino\",\n";
  out << "    \"rank_by\": \"sortino\",\n";
  out << "    \"minimum_training_trades\": " << std::max(1, config.walk_train_months / 2) << ",\n";
  out << "    \"minimum_training_win_rate\": 0.800000,\n";
  out << "    \"top_k\": 1\n";
  out << "  },\n";
  out << "  \"grid_sampling\": {\n";
  out << "    \"sample_pct\": " << config.grid.grid_sample_pct << ",\n";
  out << "    \"sample_seed\": " << config.grid.grid_sample_seed << ",\n";
  out << "    \"random_samples\": " << config.grid.grid_random_samples << "\n";
  out << "  },\n";
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
  out << "  \"regime\": {\n";
  out << "    \"fast_lookback_min\": " << config.grid.fast_lookback_min << ",\n";
  out << "    \"fast_lookback_max\": " << config.grid.fast_lookback_max << ",\n";
  out << "    \"fast_lookback_step\": " << config.grid.fast_lookback_step << ",\n";
  out << "    \"short_y_min\": " << config.grid.short_y_min << ",\n";
  out << "    \"short_y_max\": " << config.grid.short_y_max << ",\n";
  out << "    \"short_y_step\": " << config.grid.short_y_step << ",\n";
  out << "    \"short_fast_lookback_min\": " << config.grid.short_fast_lookback_min << ",\n";
  out << "    \"short_fast_lookback_max\": " << config.grid.short_fast_lookback_max << ",\n";
  out << "    \"short_fast_lookback_step\": " << config.grid.short_fast_lookback_step << ",\n";
  out << "    \"allow_short_min\": " << config.grid.allow_short_min << ",\n";
  out << "    \"allow_short_max\": " << config.grid.allow_short_max << "\n";
  out << "  },\n";
  out << "  \"risk_exits\": {\n";
  out << "    \"take_profit_min\": " << config.grid.take_profit_min << ",\n";
  out << "    \"take_profit_max\": " << config.grid.take_profit_max << ",\n";
  out << "    \"take_profit_step\": " << config.grid.take_profit_step << ",\n";
  out << "    \"stop_loss_min\": " << config.grid.stop_loss_min << ",\n";
  out << "    \"stop_loss_max\": " << config.grid.stop_loss_max << ",\n";
  out << "    \"stop_loss_step\": " << config.grid.stop_loss_step << ",\n";
  out << "    \"trailing_stop_min\": " << config.grid.trailing_stop_min << ",\n";
  out << "    \"trailing_stop_max\": " << config.grid.trailing_stop_max << ",\n";
  out << "    \"trailing_stop_step\": " << config.grid.trailing_stop_step << ",\n";
  out << "    \"regime_weak_exit_min\": " << config.grid.regime_weak_exit_min << ",\n";
  out << "    \"regime_weak_exit_max\": " << config.grid.regime_weak_exit_max << ",\n";
  out << "    \"volatility_lookback_min\": " << config.grid.volatility_lookback_min << ",\n";
  out << "    \"volatility_lookback_max\": " << config.grid.volatility_lookback_max << ",\n";
  out << "    \"volatility_lookback_step\": " << config.grid.volatility_lookback_step << ",\n";
  out << "    \"target_volatility_min\": " << config.grid.target_volatility_min << ",\n";
  out << "    \"target_volatility_max\": " << config.grid.target_volatility_max << ",\n";
  out << "    \"target_volatility_step\": " << config.grid.target_volatility_step << ",\n";
  out << "    \"max_position_pct_min\": " << config.grid.max_position_pct_min << ",\n";
  out << "    \"max_position_pct_max\": " << config.grid.max_position_pct_max << ",\n";
  out << "    \"max_position_pct_step\": " << config.grid.max_position_pct_step << "\n";
  out << "  },\n";
  out << "  \"buy_and_hold\": ";
  write_benchmark_json(out, buy_and_hold, "  ");
  out << ",\n";
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
    out << "      \"direction\": \"" << json_escape(trade.direction) << "\",\n";
    out << "      \"entry_price\": " << trade.entry_price << ",\n";
    out << "      \"exit_price\": " << trade.exit_price << ",\n";
    out << "      \"shares\": " << trade.shares << ",\n";
    out << "      \"pnl\": " << trade.pnl << ",\n";
    out << "      \"costs\": " << trade.costs << ",\n";
    out << "      \"entry_signal_quality\": " << trade.entry_signal_quality << ",\n";
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
    out << "      \"training_selection\": ";
    write_result_json(out, period.training_selection, "      ");
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

      sort_results_by_cagr(results);
      summary = results.front();
      json = to_json(results, summary, buy_and_hold, prices.size(), config);
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
    std::cout << "Fixed cost/order: " << config.costs.fixed_per_order << "\n";
    std::cout << "Variable cost rate/order: " << config.costs.variable_rate << "\n";
    std::cout << "Bars/year: " << config.annualization.bars_per_year << "\n";
    std::cout << "Buy-and-hold CAGR: " << std::fixed << std::setprecision(4) << (buy_and_hold.metrics.cagr * 100.0) << "%\n";
    if (config.mode == "walk-forward") {
      std::cout << "Walk-forward periods: " << walk_periods << "\n";
      std::cout << "Train months: " << config.walk_train_months << "\n";
      std::cout << "Test months: " << config.walk_test_months << "\n";
      std::cout << "Out-of-sample CAGR: " << std::fixed << std::setprecision(4) << (summary.metrics.cagr * 100.0) << "%\n";
    } else {
      std::cout << "Best threshold_pct: " << summary.params.diff_pct << "\n";
      std::cout << "Best lookback_days: " << summary.params.lookback_days << "\n";
      std::cout << "Best fast_lookback_days: " << summary.params.fast_lookback_days << "\n";
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
