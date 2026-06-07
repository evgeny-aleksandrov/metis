#include "metis/reporting/json_writer.hpp"

#include "metis/core/date.hpp"
#include "metis/strategy/strategy_type.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace metis {
namespace {

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

void write_config_json(std::ostringstream& out, const CliConfig& config) {
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
    const CliConfig& config) {
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
    const CliConfig& config) {
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
  write_config_json(out, config);
  out << "  \"walk_forward\": {\n";
  out << "    \"train_months\": " << config.walk_train_months << ",\n";
  out << "    \"test_months\": " << config.walk_test_months << ",\n";
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

}  // namespace metis
