#include "metis/analytics/indicators.hpp"

#include <cmath>

namespace metis {

double simple_moving_average(const std::vector<Candle>& prices, size_t end_index, int lookback) {
  if (lookback <= 0 || end_index + 1 < static_cast<size_t>(lookback)) {
    return 0.0;
  }
  double total = 0.0;
  const size_t start = end_index + 1 - static_cast<size_t>(lookback);
  for (size_t index = start; index <= end_index; ++index) {
    total += prices[index].close;
  }
  return total / static_cast<double>(lookback);
}

double realized_volatility(const std::vector<Candle>& prices, size_t end_index, int lookback, double bars_per_year) {
  if (lookback <= 1 || end_index < static_cast<size_t>(lookback)) {
    return 0.0;
  }
  std::vector<double> returns;
  returns.reserve(static_cast<size_t>(lookback));
  const size_t start = end_index + 1 - static_cast<size_t>(lookback);
  for (size_t index = start + 1; index <= end_index; ++index) {
    const double previous = prices[index - 1].close;
    returns.push_back(previous > 0.0 ? (prices[index].close / previous) - 1.0 : 0.0);
  }
  double mean = 0.0;
  for (double value : returns) {
    mean += value;
  }
  mean /= static_cast<double>(returns.size());

  double variance = 0.0;
  for (double value : returns) {
    const double diff = value - mean;
    variance += diff * diff;
  }
  variance /= static_cast<double>(returns.size());
  return std::sqrt(variance) * std::sqrt(bars_per_year);
}

}  // namespace metis
