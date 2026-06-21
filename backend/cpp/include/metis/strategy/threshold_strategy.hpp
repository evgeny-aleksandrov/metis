#pragma once

#include "metis/strategy/strategy.hpp"

namespace metis {

class ThresholdStrategy final : public Strategy {
public:
  explicit ThresholdStrategy(DiscreteGridStrategyParams params);

  std::string name() const override;
  const StrategyParams& params() const override;
  int required_lookback() const override;
  bool should_exit_on_signal_weakness(
      const std::vector<Candle>& prices,
      size_t index,
      int position_direction) const override;
  Signal signal(
      const std::vector<Candle>& prices,
      size_t index) const override;

private:
  StrategyParams params_;
};

}  // namespace metis
