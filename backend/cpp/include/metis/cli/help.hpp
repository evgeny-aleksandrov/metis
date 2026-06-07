#pragma once

#include "metis/cli/approach.hpp"

namespace metis {

void print_general_help();
void print_approach_help(ApproachType approach);
void print_strategy_help(ApproachType approach, StrategyFamily strategy);

}  // namespace metis
