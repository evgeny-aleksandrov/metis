#pragma once

#include "metis/types.hpp"

#include <string>

namespace metis {

StrategyType strategy_type_from_string(const std::string& value);
std::string strategy_type_to_string(StrategyType strategy);

}  // namespace metis
