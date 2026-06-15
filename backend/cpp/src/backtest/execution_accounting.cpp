#include "metis/backtest/execution_accounting.hpp"

#include <algorithm>

namespace metis {

double order_cost(double trade_value, const TransactionCosts& costs) {
  if (trade_value <= 0.0) {
    return 0.0;
  }
  return std::max(0.0, costs.fixed_per_order) + (std::max(0.0, costs.variable_rate) * trade_value);
}

}  // namespace metis
