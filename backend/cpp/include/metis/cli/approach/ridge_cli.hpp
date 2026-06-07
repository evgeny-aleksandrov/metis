#pragma once

#include "metis/cli/parse_common.hpp"

namespace metis {

bool parse_ridge_option(CliConfig& config, OptionCursor& cursor);
void validate_ridge_config(const CliConfig& config);

}  // namespace metis
