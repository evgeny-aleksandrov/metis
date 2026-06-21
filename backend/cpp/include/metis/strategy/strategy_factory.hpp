#pragma once

#include "metis/strategy/strategy.hpp"

#include <memory>

namespace metis {

std::unique_ptr<Strategy> make_strategy(const StrategyParams& params);
std::unique_ptr<Strategy> make_strategy(const DiscreteGridStrategyParams& params);
std::unique_ptr<Strategy> make_strategy(const RidgeStrategyParams& params);

}  // namespace metis
