#pragma once

#include "metis/types.hpp"

#include <cstddef>
#include <vector>

namespace metis {

class PositionSizingPolicy {
public:
  virtual ~PositionSizingPolicy() = default;

  virtual int required_lookback() const = 0;

  virtual double target_fraction(
      const std::vector<Candle>& prices,
      size_t index,
      double equity,
      const Annualization& annualization) const = 0;
};

class FixedFractionSizingPolicy final : public PositionSizingPolicy {
public:
  explicit FixedFractionSizingPolicy(double max_position_pct);

  int required_lookback() const override;
  double target_fraction(
      const std::vector<Candle>& prices,
      size_t index,
      double equity,
      const Annualization& annualization) const override;

private:
  double max_position_pct_;
};

class VolTargetSizingPolicy final : public PositionSizingPolicy {
public:
  VolTargetSizingPolicy(
      double max_position_pct,
      double target_volatility,
      int volatility_lookback_days);

  int required_lookback() const override;
  double target_fraction(
      const std::vector<Candle>& prices,
      size_t index,
      double equity,
      const Annualization& annualization) const override;

private:
  double max_position_pct_;
  double target_volatility_;
  int volatility_lookback_days_;
};

}  // namespace metis
