#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <vector>

namespace metis {

double simple_moving_average(const std::vector<Candle>& prices, size_t end_index, int lookback);
double realized_volatility(const std::vector<Candle>& prices, size_t end_index, int lookback, double bars_per_year);

}  // namespace metis
