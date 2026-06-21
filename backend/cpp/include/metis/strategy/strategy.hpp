#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace metis {

enum class Direction {
  Flat,
  Long,
  Short
};

struct Signal {
  Direction direction = Direction::Flat;
  double target_position_pct = 0.0;
  double confidence = 0.0;
};

class Strategy {
public:
  virtual ~Strategy() = default;

  virtual std::string name() const = 0;
  virtual const StrategyParams& params() const = 0;
  virtual int required_lookback() const = 0;
  virtual bool should_exit_on_signal_weakness(
      const std::vector<Candle>& prices,
      size_t index,
      int position_direction) const = 0;
  virtual Signal signal(
      const std::vector<Candle>& prices,
      size_t index) const = 0;
};

}  // namespace metis
