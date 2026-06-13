#pragma once

#include "metis/config/run_config.hpp"
#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

struct WalkForwardWindow {
  std::string train_start;
  std::string train_end;
  std::string test_start;
  std::string test_end;
  std::vector<Candle> training_prices;
  std::vector<Candle> test_prices;
};

class WalkForwardSplitter {
public:
  std::vector<WalkForwardWindow> split(
      const std::vector<Candle>& prices,
      const WalkForwardConfig& config) const;
};

}  // namespace metis
