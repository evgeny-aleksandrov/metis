#include "metis/data/csv_loader.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace metis {
namespace {

std::vector<std::string> split_csv_line(const std::string& line) {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) {
    fields.push_back(field);
  }
  return fields;
}

}  // namespace

std::vector<Candle> load_prices_from_csv(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open CSV file: " + path);
  }

  std::string header;
  if (!std::getline(file, header)) {
    throw std::runtime_error("CSV appears empty: " + path);
  }
  const std::vector<std::string> header_fields = split_csv_line(header);

  int date_index = -1;
  int close_index = -1;
  int adj_close_index = -1;
  for (size_t index = 0; index < header_fields.size(); ++index) {
    if (header_fields[index] == "Date") {
      date_index = static_cast<int>(index);
    } else if (header_fields[index] == "Close") {
      close_index = static_cast<int>(index);
    } else if (header_fields[index] == "Adj Close") {
      adj_close_index = static_cast<int>(index);
    }
  }

  if (date_index < 0 || (close_index < 0 && adj_close_index < 0)) {
    throw std::runtime_error("CSV requires at least Date and Close/Adj Close columns.");
  }

  const int price_index = (adj_close_index >= 0) ? adj_close_index : close_index;

  std::vector<Candle> prices;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }
    const std::vector<std::string> fields = split_csv_line(line);
    if (static_cast<int>(fields.size()) <= std::max(date_index, price_index)) {
      continue;
    }
    if (fields[price_index] == "null" || fields[price_index].empty()) {
      continue;
    }

    Candle candle;
    candle.date = fields[date_index];
    candle.close = std::stod(fields[price_index]);
    prices.push_back(candle);
  }

  if (prices.size() < 10) {
    throw std::runtime_error("Not enough price rows for backtesting.");
  }
  return prices;
}

}  // namespace metis
