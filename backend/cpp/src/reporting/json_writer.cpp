#include "metis/reporting/json_writer.hpp"

#include "metis/core/date.hpp"
#include "metis/strategy/strategy_type.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <variant>

namespace metis {
namespace {

void write_result_json(std::ostringstream& out, const SimulationResult& item, const std::string& indent) {
  out << indent << "{\n";
  out << indent << "  \"strategy\": \"" << discrete_grid_strategy_to_string(item.params.strategy) << "\",\n";
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

void write_config_json(std::ostringstream& out, const BacktestRunConfig& config) {
  if (!std::holds_alternative<DiscreteGridRunConfig>(config.approach_config)) {
    out << "  \"selection\": {\n";
    out << "    \"mode\": \"none\"\n";
    out << "  },\n";
    return;
  }

  const DiscreteGridRunConfig& discrete_grid_config = std::get<DiscreteGridRunConfig>(config.approach_config);
  const DiscreteGridSearchSpace& search_space = discrete_grid_config.search_space;
  const SearchSettings& search = discrete_grid_config.search;

  out << "  \"selection\": {\n";
  out << "    \"mode\": \"filtered-best-sortino\",\n";
  out << "    \"rank_by\": \"sortino\",\n";
  out << "    \"minimum_training_trades\": " << std::max(1, config.walk_forward.train_months / 2) << ",\n";
  out << "    \"minimum_training_win_rate\": 0.800000,\n";
  out << "    \"top_k\": 1\n";
  out << "  },\n";
  out << "  \"grid_sampling\": {\n";
  out << "    \"sample_pct\": " << search.sample_pct << ",\n";
  out << "    \"sample_seed\": " << search.seed << ",\n";
  out << "    \"random_samples\": " << search.random_samples << "\n";
  out << "  },\n";
  out << "  \"threshold_pct\": {\n";
  out << "    \"min\": " << search_space.threshold_pct.min << ",\n";
  out << "    \"max\": " << search_space.threshold_pct.max << ",\n";
  out << "    \"step\": " << search_space.threshold_pct.step << "\n";
  out << "  },\n";
  out << "  \"lookback_days\": {\n";
  out << "    \"min\": " << search_space.lookback_days.min << ",\n";
  out << "    \"max\": " << search_space.lookback_days.max << ",\n";
  out << "    \"step\": " << search_space.lookback_days.step << "\n";
  out << "  },\n";
  out << "  \"hold_days\": {\n";
  out << "    \"min\": " << search_space.hold_days.min << ",\n";
  out << "    \"max\": " << search_space.hold_days.max << ",\n";
  out << "    \"step\": " << search_space.hold_days.step << "\n";
  out << "  },\n";
  out << "  \"regime\": {\n";
  out << "    \"fast_lookback_min\": " << search_space.fast_lookback_days.min << ",\n";
  out << "    \"fast_lookback_max\": " << search_space.fast_lookback_days.max << ",\n";
  out << "    \"fast_lookback_step\": " << search_space.fast_lookback_days.step << ",\n";
  out << "    \"short_y_min\": " << search_space.short_lookback_days.min << ",\n";
  out << "    \"short_y_max\": " << search_space.short_lookback_days.max << ",\n";
  out << "    \"short_y_step\": " << search_space.short_lookback_days.step << ",\n";
  out << "    \"short_fast_lookback_min\": " << search_space.short_fast_lookback_days.min << ",\n";
  out << "    \"short_fast_lookback_max\": " << search_space.short_fast_lookback_days.max << ",\n";
  out << "    \"short_fast_lookback_step\": " << search_space.short_fast_lookback_days.step << ",\n";
  out << "    \"allow_short_min\": " << search_space.allow_short.min << ",\n";
  out << "    \"allow_short_max\": " << search_space.allow_short.max << "\n";
  out << "  },\n";
  out << "  \"risk_exits\": {\n";
  out << "    \"take_profit_min\": " << search_space.take_profit_pct.min << ",\n";
  out << "    \"take_profit_max\": " << search_space.take_profit_pct.max << ",\n";
  out << "    \"take_profit_step\": " << search_space.take_profit_pct.step << ",\n";
  out << "    \"stop_loss_min\": " << search_space.stop_loss_pct.min << ",\n";
  out << "    \"stop_loss_max\": " << search_space.stop_loss_pct.max << ",\n";
  out << "    \"stop_loss_step\": " << search_space.stop_loss_pct.step << ",\n";
  out << "    \"trailing_stop_min\": " << search_space.trailing_stop_pct.min << ",\n";
  out << "    \"trailing_stop_max\": " << search_space.trailing_stop_pct.max << ",\n";
  out << "    \"trailing_stop_step\": " << search_space.trailing_stop_pct.step << ",\n";
  out << "    \"regime_weak_exit_min\": " << search_space.regime_weak_exit.min << ",\n";
  out << "    \"regime_weak_exit_max\": " << search_space.regime_weak_exit.max << ",\n";
  out << "    \"volatility_lookback_min\": " << search_space.volatility_lookback_days.min << ",\n";
  out << "    \"volatility_lookback_max\": " << search_space.volatility_lookback_days.max << ",\n";
  out << "    \"volatility_lookback_step\": " << search_space.volatility_lookback_days.step << ",\n";
  out << "    \"target_volatility_min\": " << search_space.target_volatility.min << ",\n";
  out << "    \"target_volatility_max\": " << search_space.target_volatility.max << ",\n";
  out << "    \"target_volatility_step\": " << search_space.target_volatility.step << ",\n";
  out << "    \"max_position_pct_min\": " << search_space.max_position_pct.min << ",\n";
  out << "    \"max_position_pct_max\": " << search_space.max_position_pct.max << ",\n";
  out << "    \"max_position_pct_step\": " << search_space.max_position_pct.step << "\n";
  out << "  },\n";
}

void write_equity_curve_json(std::ostringstream& out, const std::vector<EquityPoint>& equity_curve) {
  out << "  \"equity_curve\": [\n";
  for (size_t index = 0; index < equity_curve.size(); ++index) {
    const EquityPoint& point = equity_curve[index];
    out << "    {\"date\": \"" << json_escape(point.date) << "\", \"equity\": " << point.equity << "}";
    if (index + 1 < equity_curve.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ],\n";
}

void write_trades_json(std::ostringstream& out, const std::vector<Trade>& trades, bool trailing_comma) {
  out << "  \"trades\": [\n";
  for (size_t index = 0; index < trades.size(); ++index) {
    const Trade& trade = trades[index];
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
    if (index + 1 < trades.size()) {
      out << ",";
    }
    out << "\n";
  }
  out << "  ]";
  if (trailing_comma) {
    out << ",";
  }
  out << "\n";
}

}  // namespace

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

std::string to_json(
    const std::vector<SimulationResult>& results,
    const SimulationResult& best,
    const SimulationResult& buy_and_hold,
    size_t bars,
    const BacktestRunConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.data.symbol) << "\",\n";
  out << "  \"mode\": \"walk-forward\",\n";
  out << "  \"approach\": \"" << approach_name(config) << "\",\n";
  out << "  \"strategy\": \"" << strategy_name(config) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.execution.initial_equity << ",\n";
  out << "  \"bars_per_year\": " << config.execution.annualization.bars_per_year << ",\n";
  out << "  \"transaction_costs\": {\n";
  out << "    \"fixed_per_order\": " << config.execution.costs.fixed_per_order << ",\n";
  out << "    \"variable_rate\": " << config.execution.costs.variable_rate << "\n";
  out << "  },\n";
  write_config_json(out, config);
  out << "  \"buy_and_hold\": ";
  write_benchmark_json(out, buy_and_hold, "  ");
  out << ",\n";
  out << "  \"best\": ";
  write_result_json(out, best, "  ");
  out << ",\n";
  write_equity_curve_json(out, best.equity_curve);
  write_trades_json(out, best.trades, true);
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

std::string to_walk_forward_json(
    const SimulationResult& combined,
    const SimulationResult& buy_and_hold,
    const std::vector<WalkForwardPeriod>& periods,
    size_t bars,
    const BacktestRunConfig& config) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "{\n";
  out << "  \"symbol\": \"" << json_escape(config.data.symbol) << "\",\n";
  out << "  \"mode\": \"walk-forward\",\n";
  out << "  \"approach\": \"" << approach_name(config) << "\",\n";
  out << "  \"strategy\": \"" << strategy_name(config) << "\",\n";
  out << "  \"generated_at\": \"" << now_utc_iso8601() << "\",\n";
  out << "  \"bars\": " << bars << ",\n";
  out << "  \"initial_equity\": " << config.execution.initial_equity << ",\n";
  out << "  \"bars_per_year\": " << config.execution.annualization.bars_per_year << ",\n";
  out << "  \"transaction_costs\": {\n";
  out << "    \"fixed_per_order\": " << config.execution.costs.fixed_per_order << ",\n";
  out << "    \"variable_rate\": " << config.execution.costs.variable_rate << "\n";
  out << "  },\n";
  write_config_json(out, config);
  out << "  \"walk_forward\": {\n";
  out << "    \"train_months\": " << config.walk_forward.train_months << ",\n";
  out << "    \"test_months\": " << config.walk_forward.test_months << ",\n";
  out << "    \"periods\": " << periods.size() << "\n";
  out << "  },\n";
  out << "  \"buy_and_hold\": ";
  write_benchmark_json(out, buy_and_hold, "  ");
  out << ",\n";
  out << "  \"best\": ";
  write_result_json(out, combined, "  ");
  out << ",\n";
  write_equity_curve_json(out, combined.equity_curve);
  write_trades_json(out, combined.trades, true);
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

std::string to_backtest_report_json(
    const BacktestRunConfig& config,
    size_t bars,
    const BacktestExecutionResult& result) {
  if (result.has_walk_forward) {
    return to_walk_forward_json(
        result.summary,
        result.benchmark,
        result.walk_forward_periods,
        bars,
        config);
  }
  return to_json({}, result.summary, result.benchmark, bars, config);
}

}  // namespace metis
