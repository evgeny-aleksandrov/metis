#include "metis/optimization/discrete_grid_search.hpp"

#include "metis/backtest/simulation.hpp"
#include "metis/strategy/strategy.hpp"

#include <cstddef>
#include <random>
#include <stdexcept>

namespace metis {
namespace {

std::vector<int> int_values(int min_value, int max_value, int step) {
  std::vector<int> values;
  for (int value = min_value; value <= max_value; value += step) {
    values.push_back(value);
  }
  return values;
}

std::vector<int> int_values(const Range<int>& range) {
  return int_values(range.min, range.max, range.step);
}

std::vector<double> double_values(const Range<double>& range) {
  std::vector<double> values;
  for (double value = range.min; value <= range.max + 1e-9; value += range.step) {
    values.push_back(value);
  }
  return values;
}

class SearchSampler {
public:
  explicit SearchSampler(const SearchSettings& settings)
      : settings_(settings), random_engine_(settings.seed), sample_distribution_(0.0, 1.0) {}

  bool should_evaluate_candidate() {
    return settings_.sample_pct >= 1.0 || sample_distribution_(random_engine_) <= settings_.sample_pct;
  }

  int pick_int(const std::vector<int>& values) {
    std::uniform_int_distribution<std::size_t> distribution(0, values.size() - 1);
    return values[distribution(random_engine_)];
  }

  double pick_double(const std::vector<double>& values) {
    std::uniform_int_distribution<std::size_t> distribution(0, values.size() - 1);
    return values[distribution(random_engine_)];
  }

private:
  const SearchSettings& settings_;
  std::mt19937 random_engine_;
  std::uniform_real_distribution<double> sample_distribution_;
};

std::vector<SimulationResult> run_regime_random_search(
    const std::vector<Candle>& prices,
    const DiscreteGridRunConfig& run_config,
    const ExecutionConfig& execution,
    SearchSampler& sampler) {
  const DiscreteGridSearchSpace& search_space = run_config.search_space;
  const SearchSettings& search = run_config.search;
  if (search.random_samples <= 0) {
    throw std::runtime_error("Regime search requires random_samples > 0.");
  }

  std::vector<SimulationResult> results;
  const std::vector<int> slow_values = int_values(search_space.lookback_days);
  const std::vector<int> fast_values = int_values(search_space.fast_lookback_days);
  const std::vector<int> allow_short_values = int_values(search_space.allow_short.min, search_space.allow_short.max, 1);
  const std::vector<int> short_slow_values =
      search_space.short_lookback_days.min > 0
          ? int_values(search_space.short_lookback_days)
          : std::vector<int>{0};
  const std::vector<int> short_fast_values =
      search_space.short_fast_lookback_days.min > 0
          ? int_values(search_space.short_fast_lookback_days)
          : std::vector<int>{0};
  const std::vector<int> vol_lookback_values =
      int_values(search_space.volatility_lookback_days);
  const std::vector<double> target_volatility_values =
      double_values(search_space.target_volatility);
  const std::vector<double> max_position_values =
      double_values(search_space.max_position_pct);
  const std::vector<double> take_profit_values =
      double_values(search_space.take_profit_pct);
  const std::vector<double> stop_loss_values =
      double_values(search_space.stop_loss_pct);
  const std::vector<double> trailing_stop_values =
      double_values(search_space.trailing_stop_pct);
  const std::vector<int> weak_exit_values =
      int_values(search_space.regime_weak_exit.min, search_space.regime_weak_exit.max, 1);

  int attempts = 0;
  const int max_attempts = search.random_samples * 50;
  while (static_cast<int>(results.size()) < search.random_samples && attempts < max_attempts) {
    attempts += 1;
    const int y = sampler.pick_int(slow_values);
    const int fast_lookback = sampler.pick_int(fast_values);
    if (fast_lookback >= y) {
      continue;
    }
    const int allow_short = sampler.pick_int(allow_short_values);
    const int short_y = allow_short != 0 && search_space.short_lookback_days.min > 0 ? sampler.pick_int(short_slow_values) : y;
    const int short_fast_lookback =
        allow_short != 0 && search_space.short_fast_lookback_days.min > 0 ? sampler.pick_int(short_fast_values) : fast_lookback;
    if (short_fast_lookback >= short_y) {
      continue;
    }

    StrategyParams params;
    params.strategy = run_config.strategy;
    params.lookback_days = y;
    params.fast_lookback_days = fast_lookback;
    params.short_lookback_days = short_y;
    params.short_fast_lookback_days = short_fast_lookback;
    params.take_profit_pct = sampler.pick_double(take_profit_values);
    params.stop_loss_pct = sampler.pick_double(stop_loss_values);
    params.trailing_stop_pct = sampler.pick_double(trailing_stop_values);
    params.exit_on_regime_weakness = sampler.pick_int(weak_exit_values) != 0;
    params.allow_short = allow_short != 0;
    params.volatility_lookback_days = sampler.pick_int(vol_lookback_values);
    params.target_volatility = sampler.pick_double(target_volatility_values);
    params.max_position_pct = sampler.pick_double(max_position_values);

    const DiscreteStrategy candidate(params);
    results.push_back(run_simulation(prices, candidate, execution));
  }
  return results;
}

std::vector<SimulationResult> run_threshold_grid_search(
    const std::vector<Candle>& prices,
    const DiscreteGridRunConfig& run_config,
    const ExecutionConfig& execution,
    SearchSampler& sampler) {
  const DiscreteGridSearchSpace& search_space = run_config.search_space;
  std::vector<SimulationResult> results;
  for (double x = search_space.threshold_pct.min; x <= search_space.threshold_pct.max + 1e-9; x += search_space.threshold_pct.step) {
    for (int y = search_space.lookback_days.min; y <= search_space.lookback_days.max; y += search_space.lookback_days.step) {
      for (int hold_days = search_space.hold_days.min; hold_days <= search_space.hold_days.max; hold_days += search_space.hold_days.step) {
        for (double take_profit = search_space.take_profit_pct.min; take_profit <= search_space.take_profit_pct.max + 1e-9; take_profit += search_space.take_profit_pct.step) {
          for (double stop_loss = search_space.stop_loss_pct.min; stop_loss <= search_space.stop_loss_pct.max + 1e-9; stop_loss += search_space.stop_loss_pct.step) {
            for (double trailing_stop = search_space.trailing_stop_pct.min; trailing_stop <= search_space.trailing_stop_pct.max + 1e-9; trailing_stop += search_space.trailing_stop_pct.step) {
              StrategyParams params;
              params.strategy = run_config.strategy;
              params.diff_pct = x;
              params.lookback_days = y;
              params.hold_days = hold_days;
              params.take_profit_pct = take_profit;
              params.stop_loss_pct = stop_loss;
              params.trailing_stop_pct = trailing_stop;

              if (sampler.should_evaluate_candidate()) {
                const DiscreteStrategy candidate(params);
                results.push_back(run_simulation(prices, candidate, execution));
              }
            }
          }
        }
      }
    }
  }
  return results;
}

}  // namespace

DiscreteGridSearchOptimizer::DiscreteGridSearchOptimizer(DiscreteGridRunConfig config) : config_(config) {}

std::vector<SimulationResult> DiscreteGridSearchOptimizer::evaluate(
    const std::vector<Candle>& prices,
    const ExecutionConfig& execution) const {
  return run_search(prices, config_, execution);
}

std::vector<SimulationResult> run_search(
    const std::vector<Candle>& prices,
    const DiscreteGridRunConfig& run_config,
    const ExecutionConfig& execution) {
  SearchSampler sampler(run_config.search);
  if (run_config.strategy == DiscreteGridStrategy::Regime) {
    return run_regime_random_search(prices, run_config, execution, sampler);
  }
  return run_threshold_grid_search(prices, run_config, execution, sampler);
}

}  // namespace metis
