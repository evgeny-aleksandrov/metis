#pragma once

#include "metis/cli/parse_common.hpp"

namespace metis {

bool parse_discrete_grid_option(CliConfig& config, OptionCursor& cursor);
void validate_discrete_grid_config(const CliConfig& config);

}  // namespace metis
