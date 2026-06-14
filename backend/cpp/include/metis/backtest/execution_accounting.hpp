#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <vector>

namespace metis {

double order_cost(double trade_value, const TransactionCosts& costs);

double position_fraction(
    const std::vector<Candle>& prices,
    size_t index,
    double max_position_pct,
    double target_volatility,
    int volatility_lookback_days,
    const Annualization& annualization);

}  // namespace metis
