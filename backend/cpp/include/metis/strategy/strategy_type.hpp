#pragma once

#include "metis/types.hpp"

#include <string>

namespace metis {

DiscreteGridStrategy discrete_grid_strategy_from_string(const std::string& value);
std::string discrete_grid_strategy_to_string(DiscreteGridStrategy strategy);

}  // namespace metis
