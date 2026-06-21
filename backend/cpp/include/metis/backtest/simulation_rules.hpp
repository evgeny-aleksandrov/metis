#pragma once

#include "metis/backtest/position_sizing_policy.hpp"
#include "metis/backtest/trade_exit_policy.hpp"

#include <memory>

namespace metis {

struct SimulationRules {
  std::unique_ptr<PositionSizingPolicy> sizing_policy;
  std::unique_ptr<TradeExitPolicy> exit_policy;
};

}  // namespace metis
