#pragma once

#include "metis/types.hpp"

#include <string>
#include <vector>

namespace metis {

std::vector<Candle> load_prices_from_csv(const std::string& path);

}  // namespace metis
