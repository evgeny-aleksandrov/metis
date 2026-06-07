#include "metis/core/date.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace metis {

int days_in_month(int year, int month) {
  static const int days_by_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2) {
    const bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    return leap ? 29 : 28;
  }
  return days_by_month[month - 1];
}

DateParts parse_date(const std::string& value) {
  if (value.size() < 10) {
    throw std::runtime_error("Expected date in YYYY-MM-DD format: " + value);
  }
  return {std::stoi(value.substr(0, 4)), std::stoi(value.substr(5, 2)), std::stoi(value.substr(8, 2))};
}

std::string format_date(const DateParts& date) {
  std::ostringstream out;
  out << std::setw(4) << std::setfill('0') << date.year << "-"
      << std::setw(2) << std::setfill('0') << date.month << "-"
      << std::setw(2) << std::setfill('0') << date.day;
  return out.str();
}

std::string add_months(const std::string& value, int months) {
  DateParts date = parse_date(value);
  const int zero_based_month = (date.month - 1) + months;
  date.year += zero_based_month / 12;
  date.month = (zero_based_month % 12) + 1;
  if (date.month <= 0) {
    date.month += 12;
    date.year -= 1;
  }
  const int max_day = days_in_month(date.year, date.month);
  if (date.day > max_day) {
    date.day = max_day;
  }
  return format_date(date);
}

std::string now_utc_iso8601() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm{};
#ifdef _WIN32
  gmtime_s(&utc_tm, &now_time);
#else
  gmtime_r(&now_time, &utc_tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

std::string now_utc_compact() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm utc_tm{};
#ifdef _WIN32
  gmtime_s(&utc_tm, &now_time);
#else
  gmtime_r(&now_time, &utc_tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&utc_tm, "%Y%m%d-%H%M%SZ");
  return oss.str();
}

}  // namespace metis
