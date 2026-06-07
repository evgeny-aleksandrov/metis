#pragma once

#include <string>

namespace metis {

struct DateParts {
  int year = 0;
  int month = 0;
  int day = 0;
};

int days_in_month(int year, int month);
DateParts parse_date(const std::string& value);
std::string format_date(const DateParts& date);
std::string add_months(const std::string& value, int months);
std::string now_utc_iso8601();
std::string now_utc_compact();

}  // namespace metis
