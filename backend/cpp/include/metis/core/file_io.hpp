#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace metis {

std::ofstream open_output_file(const std::filesystem::path& output_path, const std::string& description);
void write_text_file(
    const std::filesystem::path& output_path,
    const std::string& contents,
    const std::string& description = "output file");

}  // namespace metis
