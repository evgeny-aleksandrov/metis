#include "metis/backtest/trade_exit_policy.hpp"

#include <algorithm>

namespace metis {
namespace {

double position_return(const OpenPosition& position, double close) {
  if (position.entry_price <= 0.0) {
    return 0.0;
  }
  return static_cast<double>(position.direction) * ((close / position.entry_price) - 1.0);
}

}  // namespace

HoldPeriodExitPolicy::HoldPeriodExitPolicy(int hold_days) : hold_days_(hold_days) {}

void HoldPeriodExitPolicy::on_entry(const OpenPosition& /*position*/) {}

ExitDecision HoldPeriodExitPolicy::evaluate(
    const std::vector<Candle>& /*prices*/,
    size_t /*index*/,
    const OpenPosition& position) {
  return {position.held_days >= hold_days_, "hold-period"};
}

TakeProfitExitPolicy::TakeProfitExitPolicy(double take_profit_pct) : take_profit_pct_(take_profit_pct) {}

void TakeProfitExitPolicy::on_entry(const OpenPosition& /*position*/) {}

ExitDecision TakeProfitExitPolicy::evaluate(
    const std::vector<Candle>& prices,
    size_t index,
    const OpenPosition& position) {
  const double trade_return = position_return(position, prices[index].close);
  return {trade_return >= take_profit_pct_, "take-profit"};
}

StopLossExitPolicy::StopLossExitPolicy(double stop_loss_pct) : stop_loss_pct_(stop_loss_pct) {}

void StopLossExitPolicy::on_entry(const OpenPosition& /*position*/) {}

ExitDecision StopLossExitPolicy::evaluate(
    const std::vector<Candle>& prices,
    size_t index,
    const OpenPosition& position) {
  const double trade_return = position_return(position, prices[index].close);
  return {trade_return <= -stop_loss_pct_, "stop-loss"};
}

TrailingStopExitPolicy::TrailingStopExitPolicy(double trailing_stop_pct, double activation_return_pct)
    : trailing_stop_pct_(trailing_stop_pct), activation_return_pct_(activation_return_pct) {}

void TrailingStopExitPolicy::on_entry(const OpenPosition& position) {
  highest_price_since_entry_ = position.entry_price;
  lowest_price_since_entry_ = position.entry_price;
  active_ = false;
}

ExitDecision TrailingStopExitPolicy::evaluate(
    const std::vector<Candle>& prices,
    size_t index,
    const OpenPosition& position) {
  const double close = prices[index].close;
  if (close > highest_price_since_entry_) {
    highest_price_since_entry_ = close;
  }
  if (lowest_price_since_entry_ <= 0.0 || close < lowest_price_since_entry_) {
    lowest_price_since_entry_ = close;
  }

  const double trade_return = position_return(position, close);
  if (activation_return_pct_ > 0.0 && trade_return >= activation_return_pct_) {
    active_ = true;
  }
  if (!active_) {
    return {};
  }

  const bool long_exit =
      position.direction > 0 &&
      highest_price_since_entry_ > 0.0 &&
      close <= highest_price_since_entry_ * (1.0 - trailing_stop_pct_);
  const bool short_exit =
      position.direction < 0 &&
      lowest_price_since_entry_ > 0.0 &&
      close >= lowest_price_since_entry_ * (1.0 + trailing_stop_pct_);
  return {long_exit || short_exit, "trailing-stop"};
}

SignalWeaknessExitPolicy::SignalWeaknessExitPolicy(const Strategy& strategy) : strategy_(strategy) {}

void SignalWeaknessExitPolicy::on_entry(const OpenPosition& /*position*/) {}

ExitDecision SignalWeaknessExitPolicy::evaluate(
    const std::vector<Candle>& prices,
    size_t index,
    const OpenPosition& position) {
  return {
      strategy_.should_exit_on_signal_weakness(prices, index, position.direction),
      "signal-weakness"};
}

void CompositeExitPolicy::add(std::unique_ptr<TradeExitPolicy> policy) {
  policies_.push_back(std::move(policy));
}

void CompositeExitPolicy::on_entry(const OpenPosition& position) {
  for (const std::unique_ptr<TradeExitPolicy>& policy : policies_) {
    policy->on_entry(position);
  }
}

ExitDecision CompositeExitPolicy::evaluate(
    const std::vector<Candle>& prices,
    size_t index,
    const OpenPosition& position) {
  for (const std::unique_ptr<TradeExitPolicy>& policy : policies_) {
    const ExitDecision decision = policy->evaluate(prices, index, position);
    if (decision.should_exit) {
      return decision;
    }
  }
  return {};
}

std::unique_ptr<TradeExitPolicy> create_trade_exit_policy(
    const TradeManagementRules& rules,
    const Strategy& strategy) {
  auto policy = std::make_unique<CompositeExitPolicy>();
  if (rules.use_timed_exit && rules.hold_days > 0) {
    policy->add(std::make_unique<HoldPeriodExitPolicy>(rules.hold_days));
  }
  if (rules.take_profit_pct > 0.0 && rules.trailing_stop_pct <= 0.0) {
    policy->add(std::make_unique<TakeProfitExitPolicy>(rules.take_profit_pct));
  }
  if (rules.stop_loss_pct > 0.0) {
    policy->add(std::make_unique<StopLossExitPolicy>(rules.stop_loss_pct));
  }
  if (rules.trailing_stop_pct > 0.0) {
    policy->add(std::make_unique<TrailingStopExitPolicy>(
        rules.trailing_stop_pct,
        rules.take_profit_pct));
  }
  if (rules.use_signal_weakness_exit) {
    policy->add(std::make_unique<SignalWeaknessExitPolicy>(strategy));
  }
  return policy;
}

}  // namespace metis
