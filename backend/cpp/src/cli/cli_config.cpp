#include "metis/cli/cli_config.hpp"

#include "metis/strategy/strategy_type.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace metis {

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

CliConfig parse_args(int argc, char** argv) {
  CliConfig config;
  for (int index = 1; index < argc; ++index) {
    const std::string key = argv[index];
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

}  // namespace metis
