#include "metis/reporting/training_results_writer.hpp"

#include "metis/core/file_io.hpp"
#include "metis/strategy/strategy_type.hpp"

#include <iomanip>
#include <sstream>

namespace metis {

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
  std::ostringstream filename;
  filename << "period_" << std::setw(3) << std::setfill('0') << period_index
           << "_train_" << sanitize_path_part(train_start)
           << "_to_" << sanitize_path_part(train_end)
           << "_test_" << sanitize_path_part(test_start)
           << "_to_" << sanitize_path_part(test_end)
           << ".csv";
  const std::filesystem::path output_path = run_dir / filename.str();
  std::ofstream out = open_output_file(output_path, "training results");
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

}  // namespace metis
