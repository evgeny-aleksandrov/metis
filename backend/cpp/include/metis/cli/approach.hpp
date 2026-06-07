#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <string>

namespace metis {

ApproachType approach_type_from_string(const std::string& value);
std::string approach_type_to_string(ApproachType approach);

StrategyFamily strategy_family_from_string(ApproachType approach, const std::string& value);
std::string strategy_family_to_string(StrategyFamily strategy);

bool is_strategy_supported_by_approach(ApproachType approach, StrategyFamily strategy);
StrategyType discrete_strategy_type_from_family(StrategyFamily strategy);

}  // namespace metis
