#include "metis/cli/approach/discrete_grid_cli.hpp"

#include <stdexcept>
#include <variant>

namespace metis {
namespace {

DiscreteGridCliOptions& discrete_grid_options(CliConfig& config) {
  return std::get<DiscreteGridCliOptions>(config.approach_options);
}

const DiscreteGridCliOptions& discrete_grid_options(const CliConfig& config) {
  return std::get<DiscreteGridCliOptions>(config.approach_options);
}

void parse_regime_weak_exit_mode(DiscreteGridCliOptions& options, const std::string& mode) {
  if (mode == "off") {
    options.search_space.regime_weak_exit.min = 0;
    options.search_space.regime_weak_exit.max = 0;
  } else if (mode == "on") {
    options.search_space.regime_weak_exit.min = 1;
    options.search_space.regime_weak_exit.max = 1;
  } else if (mode == "both") {
    options.search_space.regime_weak_exit.min = 0;
    options.search_space.regime_weak_exit.max = 1;
  } else {
    throw std::runtime_error("Regime weak exit mode must be off, on, or both.");
  }
}

void parse_short_mode(DiscreteGridCliOptions& options, const std::string& mode) {
  if (mode == "off") {
    options.search_space.allow_short.min = 0;
    options.search_space.allow_short.max = 0;
  } else if (mode == "on") {
    options.search_space.allow_short.min = 1;
    options.search_space.allow_short.max = 1;
  } else if (mode == "both") {
    options.search_space.allow_short.min = 0;
    options.search_space.allow_short.max = 1;
  } else {
    throw std::runtime_error("Short mode must be off, on, or both.");
  }
}

template <typename T>
bool range_min_exceeds_max(const Range<T>& range) {
  return range.min > range.max;
}

}  // namespace

bool parse_discrete_grid_option(CliConfig& config, OptionCursor& cursor) {
  const std::string key = cursor.current();
  DiscreteGridCliOptions& options = discrete_grid_options(config);
  DiscreteGridSearchSpace& search_space = options.search_space;
  if (key == "--x-min") {
    search_space.threshold_pct.min = std::stod(cursor.need_value(key));
  } else if (key == "--x-max") {
    search_space.threshold_pct.max = std::stod(cursor.need_value(key));
  } else if (key == "--x-step") {
    search_space.threshold_pct.step = std::stod(cursor.need_value(key));
  } else if (key == "--y-min") {
    search_space.lookback_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--y-max") {
    search_space.lookback_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--y-step") {
    search_space.lookback_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-min") {
    search_space.fast_lookback_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-max") {
    search_space.fast_lookback_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-step") {
    search_space.fast_lookback_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-min") {
    search_space.short_lookback_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-max") {
    search_space.short_lookback_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-step") {
    search_space.short_lookback_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-min") {
    search_space.short_fast_lookback_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-max") {
    search_space.short_fast_lookback_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-step") {
    search_space.short_fast_lookback_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-min") {
    search_space.hold_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-max") {
    search_space.hold_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-step") {
    search_space.hold_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--take-profit-min") {
    search_space.take_profit_pct.min = std::stod(cursor.need_value(key));
  } else if (key == "--take-profit-max") {
    search_space.take_profit_pct.max = std::stod(cursor.need_value(key));
  } else if (key == "--take-profit-step") {
    search_space.take_profit_pct.step = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-min") {
    search_space.stop_loss_pct.min = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-max") {
    search_space.stop_loss_pct.max = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-step") {
    search_space.stop_loss_pct.step = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-min") {
    search_space.trailing_stop_pct.min = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-max") {
    search_space.trailing_stop_pct.max = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-step") {
    search_space.trailing_stop_pct.step = std::stod(cursor.need_value(key));
  } else if (key == "--regime-weak-exit-mode") {
    parse_regime_weak_exit_mode(options, cursor.need_value(key));
  } else if (key == "--short-mode") {
    parse_short_mode(options, cursor.need_value(key));
  } else if (key == "--vol-lookback-min") {
    search_space.volatility_lookback_days.min = std::stoi(cursor.need_value(key));
  } else if (key == "--vol-lookback-max") {
    search_space.volatility_lookback_days.max = std::stoi(cursor.need_value(key));
  } else if (key == "--vol-lookback-step") {
    search_space.volatility_lookback_days.step = std::stoi(cursor.need_value(key));
  } else if (key == "--target-vol-min") {
    search_space.target_volatility.min = std::stod(cursor.need_value(key));
  } else if (key == "--target-vol-max") {
    search_space.target_volatility.max = std::stod(cursor.need_value(key));
  } else if (key == "--target-vol-step") {
    search_space.target_volatility.step = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-min") {
    search_space.max_position_pct.min = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-max") {
    search_space.max_position_pct.max = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-step") {
    search_space.max_position_pct.step = std::stod(cursor.need_value(key));
  } else if (key == "--grid-sample-pct") {
    options.search.sample_pct = std::stod(cursor.need_value(key));
  } else if (key == "--grid-sample-seed") {
    options.search.seed = static_cast<unsigned int>(std::stoul(cursor.need_value(key)));
  } else if (key == "--grid-random-samples") {
    options.search.random_samples = std::stoi(cursor.need_value(key));
  } else {
    return false;
  }
  return true;
}

void validate_discrete_grid_config(const CliConfig& config) {
  const DiscreteGridCliOptions& options = discrete_grid_options(config);
  const DiscreteGridSearchSpace& search_space = options.search_space;
  if (search_space.threshold_pct.step <= 0.0) {
    throw std::runtime_error("X grid values must be positive.");
  }
  if (search_space.threshold_pct.min < 0.0 || search_space.threshold_pct.max < 0.0) {
    throw std::runtime_error("X threshold values must be non-negative.");
  }
  if (search_space.lookback_days.min <= 0 || search_space.lookback_days.max <= 0 || search_space.lookback_days.step <= 0 ||
      search_space.fast_lookback_days.min <= 0 || search_space.fast_lookback_days.max <= 0 || search_space.fast_lookback_days.step <= 0 ||
      search_space.short_lookback_days.min < 0 || search_space.short_lookback_days.max < 0 || search_space.short_lookback_days.step <= 0 ||
      search_space.short_fast_lookback_days.min < 0 || search_space.short_fast_lookback_days.max < 0 ||
      search_space.short_fast_lookback_days.step <= 0 ||
      search_space.hold_days.min <= 0 || search_space.hold_days.max <= 0 || search_space.hold_days.step <= 0 ||
      search_space.volatility_lookback_days.min < 0 || search_space.volatility_lookback_days.max < 0 ||
      search_space.volatility_lookback_days.step <= 0) {
    throw std::runtime_error("Lookback/hold values must be positive integers.");
  }
  if (search_space.take_profit_pct.step <= 0.0 || search_space.stop_loss_pct.step <= 0.0 ||
      search_space.trailing_stop_pct.step <= 0.0 || search_space.target_volatility.step <= 0.0 ||
      search_space.max_position_pct.step <= 0.0 || search_space.take_profit_pct.min < 0.0 ||
      search_space.take_profit_pct.max < 0.0 || search_space.stop_loss_pct.min < 0.0 ||
      search_space.stop_loss_pct.max < 0.0 || search_space.trailing_stop_pct.min < 0.0 ||
      search_space.trailing_stop_pct.max < 0.0 || search_space.target_volatility.min < 0.0 ||
      search_space.target_volatility.max < 0.0 || search_space.max_position_pct.min < 0.0 ||
      search_space.max_position_pct.max < 0.0) {
    throw std::runtime_error("Risk sizing values must be non-negative and steps must be positive.");
  }
  if (options.search.sample_pct <= 0.0 || options.search.sample_pct > 1.0) {
    throw std::runtime_error("Grid sample percent must be > 0 and <= 1.");
  }
  if (options.search.random_samples < 0) {
    throw std::runtime_error("Grid random samples cannot be negative.");
  }
  if (range_min_exceeds_max(search_space.threshold_pct) ||
      range_min_exceeds_max(search_space.lookback_days) ||
      range_min_exceeds_max(search_space.fast_lookback_days) ||
      range_min_exceeds_max(search_space.short_lookback_days) ||
      range_min_exceeds_max(search_space.short_fast_lookback_days) ||
      range_min_exceeds_max(search_space.hold_days) ||
      range_min_exceeds_max(search_space.take_profit_pct) ||
      range_min_exceeds_max(search_space.stop_loss_pct) ||
      range_min_exceeds_max(search_space.trailing_stop_pct) ||
      range_min_exceeds_max(search_space.regime_weak_exit) ||
      range_min_exceeds_max(search_space.allow_short) ||
      range_min_exceeds_max(search_space.volatility_lookback_days) ||
      range_min_exceeds_max(search_space.target_volatility) ||
      range_min_exceeds_max(search_space.max_position_pct)) {
    throw std::runtime_error("Grid min cannot exceed max.");
  }
}

}  // namespace metis
