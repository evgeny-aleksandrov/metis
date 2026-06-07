#pragma once

#include "metis/cli/parse_common.hpp"

namespace metis {

bool parse_buy_and_hold_option(CliConfig& config, OptionCursor& cursor);
void validate_buy_and_hold_config(const CliConfig& config);

}  // namespace metis
