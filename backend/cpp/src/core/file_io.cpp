#include "metis/core/file_io.hpp"

#include <stdexcept>

namespace metis {

std::ofstream open_output_file(const std::filesystem::path& output_path, const std::string& description) {
  if (!output_path.parent_path().empty()) {
    std::filesystem::create_directories(output_path.parent_path());
  }
  std::ofstream output(output_path);
  if (!output.is_open()) {
    throw std::runtime_error("Could not write " + description + ": " + output_path.string());
  }
  return output;
}

void write_text_file(
    const std::filesystem::path& output_path,
    const std::string& contents,
    const std::string& description) {
  std::ofstream output = open_output_file(output_path, description);
  output << contents;
}

}  // namespace metis
