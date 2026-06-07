#include "metis/cli/approach/buy_and_hold_cli.hpp"

#include <stdexcept>

namespace metis {

bool parse_buy_and_hold_option(CliConfig& /*config*/, OptionCursor& /*cursor*/) {
  return false;
}

void validate_buy_and_hold_config(const CliConfig& config) {
  if (config.strategy != StrategyFamily::LumpSumBuyAndHold) {
    throw std::runtime_error("Only buy-and-hold lump-sum is implemented right now.");
  }
}

}  // namespace metis
