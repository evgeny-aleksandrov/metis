#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <vector>

namespace metis {

double order_cost(double trade_value, const TransactionCosts& costs);

double position_fraction(
    const std::vector<Candle>& prices,
    size_t index,
    const StrategyParams& params,
    const Annualization& annualization);

}  // namespace metis
