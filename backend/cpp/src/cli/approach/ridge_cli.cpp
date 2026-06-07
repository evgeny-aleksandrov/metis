#include "metis/cli/approach/ridge_cli.hpp"

#include <stdexcept>

namespace metis {

bool parse_ridge_option(CliConfig& /*config*/, OptionCursor& /*cursor*/) {
  return false;
}

void validate_ridge_config(const CliConfig& /*config*/) {
  throw std::runtime_error("The ridge approach is recognized by the CLI but execution is not implemented yet.");
}

}  // namespace metis
