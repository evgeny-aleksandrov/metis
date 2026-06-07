#pragma once

#include "metis/cli/cli_config.hpp"

#include <string>

namespace metis {

struct OptionCursor {
  int argc = 0;
  char** argv = nullptr;
  int index = 0;

  std::string current() const;
  std::string need_value(const std::string& option);
};

bool is_help_token(const std::string& value);
bool parse_common_option(CliConfig& config, OptionCursor& cursor);
void validate_common_config(const CliConfig& config);

}  // namespace metis
