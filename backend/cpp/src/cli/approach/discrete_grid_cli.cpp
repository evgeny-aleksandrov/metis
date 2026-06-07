#include "metis/cli/approach/discrete_grid_cli.hpp"

#include <stdexcept>

namespace metis {
namespace {

void parse_regime_weak_exit_mode(CliConfig& config, const std::string& mode) {
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
}

void parse_short_mode(CliConfig& config, const std::string& mode) {
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
}

}  // namespace

bool parse_discrete_grid_option(CliConfig& config, OptionCursor& cursor) {
  const std::string key = cursor.current();
  if (key == "--x-min") {
    config.grid.x_min = std::stod(cursor.need_value(key));
  } else if (key == "--x-max") {
    config.grid.x_max = std::stod(cursor.need_value(key));
  } else if (key == "--x-step") {
    config.grid.x_step = std::stod(cursor.need_value(key));
  } else if (key == "--y-min") {
    config.grid.y_min = std::stoi(cursor.need_value(key));
  } else if (key == "--y-max") {
    config.grid.y_max = std::stoi(cursor.need_value(key));
  } else if (key == "--y-step") {
    config.grid.y_step = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-min") {
    config.grid.fast_lookback_min = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-max") {
    config.grid.fast_lookback_max = std::stoi(cursor.need_value(key));
  } else if (key == "--fast-lookback-step") {
    config.grid.fast_lookback_step = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-min") {
    config.grid.short_y_min = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-max") {
    config.grid.short_y_max = std::stoi(cursor.need_value(key));
  } else if (key == "--short-y-step") {
    config.grid.short_y_step = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-min") {
    config.grid.short_fast_lookback_min = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-max") {
    config.grid.short_fast_lookback_max = std::stoi(cursor.need_value(key));
  } else if (key == "--short-fast-lookback-step") {
    config.grid.short_fast_lookback_step = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-min") {
    config.grid.hold_days_min = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-max") {
    config.grid.hold_days_max = std::stoi(cursor.need_value(key));
  } else if (key == "--hold-step") {
    config.grid.hold_days_step = std::stoi(cursor.need_value(key));
  } else if (key == "--take-profit-min") {
    config.grid.take_profit_min = std::stod(cursor.need_value(key));
  } else if (key == "--take-profit-max") {
    config.grid.take_profit_max = std::stod(cursor.need_value(key));
  } else if (key == "--take-profit-step") {
    config.grid.take_profit_step = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-min") {
    config.grid.stop_loss_min = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-max") {
    config.grid.stop_loss_max = std::stod(cursor.need_value(key));
  } else if (key == "--stop-loss-step") {
    config.grid.stop_loss_step = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-min") {
    config.grid.trailing_stop_min = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-max") {
    config.grid.trailing_stop_max = std::stod(cursor.need_value(key));
  } else if (key == "--trailing-stop-step") {
    config.grid.trailing_stop_step = std::stod(cursor.need_value(key));
  } else if (key == "--regime-weak-exit-mode") {
    parse_regime_weak_exit_mode(config, cursor.need_value(key));
  } else if (key == "--short-mode") {
    parse_short_mode(config, cursor.need_value(key));
  } else if (key == "--vol-lookback-min") {
    config.grid.volatility_lookback_min = std::stoi(cursor.need_value(key));
  } else if (key == "--vol-lookback-max") {
    config.grid.volatility_lookback_max = std::stoi(cursor.need_value(key));
  } else if (key == "--vol-lookback-step") {
    config.grid.volatility_lookback_step = std::stoi(cursor.need_value(key));
  } else if (key == "--target-vol-min") {
    config.grid.target_volatility_min = std::stod(cursor.need_value(key));
  } else if (key == "--target-vol-max") {
    config.grid.target_volatility_max = std::stod(cursor.need_value(key));
  } else if (key == "--target-vol-step") {
    config.grid.target_volatility_step = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-min") {
    config.grid.max_position_pct_min = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-max") {
    config.grid.max_position_pct_max = std::stod(cursor.need_value(key));
  } else if (key == "--max-position-pct-step") {
    config.grid.max_position_pct_step = std::stod(cursor.need_value(key));
  } else if (key == "--grid-sample-pct") {
    config.grid.grid_sample_pct = std::stod(cursor.need_value(key));
  } else if (key == "--grid-sample-seed") {
    config.grid.grid_sample_seed = static_cast<unsigned int>(std::stoul(cursor.need_value(key)));
  } else if (key == "--grid-random-samples") {
    config.grid.grid_random_samples = std::stoi(cursor.need_value(key));
  } else {
    return false;
  }
  return true;
}

void validate_discrete_grid_config(const CliConfig& config) {
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
}

}  // namespace metis
