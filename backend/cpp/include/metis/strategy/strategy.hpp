#pragma once

#include "metis/types.hpp"

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

struct PortfolioState {
  double equity = 0.0;
  int position_direction = 0;
};

class Strategy {
public:
  virtual ~Strategy() = default;

  virtual std::string name() const = 0;
  virtual const StrategyParams& params() const = 0;
  virtual int required_lookback() const = 0;
  virtual bool uses_timed_exit() const = 0;
  virtual bool should_exit_on_signal_weakness(
      const std::vector<Candle>& prices,
      size_t index,
      int position_direction) const = 0;
  virtual Signal signal(
      const std::vector<Candle>& prices,
      size_t index,
      const PortfolioState& state) const = 0;
};

class DiscreteStrategy final : public Strategy {
public:
  explicit DiscreteStrategy(StrategyParams params);

  std::string name() const override;
  const StrategyParams& params() const override;
  int required_lookback() const override;
  bool uses_timed_exit() const override;
  bool should_exit_on_signal_weakness(
      const std::vector<Candle>& prices,
      size_t index,
      int position_direction) const override;
  Signal signal(
      const std::vector<Candle>& prices,
      size_t index,
      const PortfolioState& state) const override;

private:
  StrategyParams params_;
};

}  // namespace metis
