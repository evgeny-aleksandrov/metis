#pragma once

#include "metis/strategy/strategy.hpp"
#include "metis/types.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace metis {

struct OpenPosition {
  double entry_value = 0.0;
  double entry_price = 0.0;
  double shares = 0.0;
  double entry_signal_quality = 0.0;
  std::string entry_date;
  int direction = 0;
  int held_days = 0;

  bool is_open() const {
    return shares > 0.0;
  }
};

struct ExitDecision {
  bool should_exit = false;
  std::string reason;
};

class TradeExitPolicy {
public:
  virtual ~TradeExitPolicy() = default;

  virtual void on_entry(const OpenPosition& position) = 0;

  virtual ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) = 0;
};

class HoldPeriodExitPolicy final : public TradeExitPolicy {
public:
  explicit HoldPeriodExitPolicy(int hold_days);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  int hold_days_;
};

class TakeProfitExitPolicy final : public TradeExitPolicy {
public:
  explicit TakeProfitExitPolicy(double take_profit_pct);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  double take_profit_pct_;
};

class StopLossExitPolicy final : public TradeExitPolicy {
public:
  explicit StopLossExitPolicy(double stop_loss_pct);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  double stop_loss_pct_;
};

class TrailingStopExitPolicy final : public TradeExitPolicy {
public:
  TrailingStopExitPolicy(double trailing_stop_pct, double activation_return_pct);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  double trailing_stop_pct_;
  double activation_return_pct_;
  double highest_price_since_entry_ = 0.0;
  double lowest_price_since_entry_ = 0.0;
  bool active_ = false;
};

class SignalWeaknessExitPolicy final : public TradeExitPolicy {
public:
  explicit SignalWeaknessExitPolicy(const Strategy& strategy);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  const Strategy& strategy_;
};

class CompositeExitPolicy final : public TradeExitPolicy {
public:
  void add(std::unique_ptr<TradeExitPolicy> policy);

  void on_entry(const OpenPosition& position) override;
  ExitDecision evaluate(
      const std::vector<Candle>& prices,
      size_t index,
      const OpenPosition& position) override;

private:
  std::vector<std::unique_ptr<TradeExitPolicy>> policies_;
};

}  // namespace metis
