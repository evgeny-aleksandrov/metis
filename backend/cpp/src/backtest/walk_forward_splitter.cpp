#include "metis/backtest/walk_forward_splitter.hpp"

#include "metis/core/date.hpp"

#include <stdexcept>
#include <utility>

namespace metis {
namespace {

std::vector<Candle> slice_prices(const std::vector<Candle>& prices, const std::string& start, const std::string& end) {
  std::vector<Candle> slice;
  for (const Candle& candle : prices) {
    if (candle.date >= start && candle.date < end) {
      slice.push_back(candle);
    }
  }
  return slice;
}

}  // namespace

std::vector<WalkForwardWindow> WalkForwardSplitter::split(
    const std::vector<Candle>& prices,
    const WalkForwardConfig& config) const {
  if (prices.empty()) {
    throw std::runtime_error("Cannot split walk-forward windows without prices.");
  }

  std::vector<WalkForwardWindow> windows;
  std::string test_start = add_months(prices.front().date, config.train_months);
  while (test_start < prices.back().date) {
    const std::string train_start = add_months(test_start, -config.train_months);
    const std::string test_end = add_months(test_start, config.test_months);
    if (test_end > prices.back().date) {
      break;
    }

    std::vector<Candle> training_prices = slice_prices(prices, train_start, test_start);
    std::vector<Candle> test_prices = slice_prices(prices, test_start, test_end);
    if (training_prices.size() < 30 || test_prices.size() < 30) {
      break;
    }

    windows.push_back({
        training_prices.front().date,
        training_prices.back().date,
        test_prices.front().date,
        test_prices.back().date,
        std::move(training_prices),
        std::move(test_prices)});

    test_start = test_end;
  }
  return windows;
}

}  // namespace metis
