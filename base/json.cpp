#include "base/json.h"

#include <fstream>
#include <sstream>

std::optional<boost::json::value> LoadJsonFromFile(
    const std::filesystem::path& path,
    std::string* error_message) {
  std::ifstream ifs{path};
  if (!ifs) {
    if (error_message)
      *error_message = "Cannot open file";
    return std::nullopt;
  }
  std::string contents{std::istreambuf_iterator<char>{ifs}, {}};
  return LoadJsonFromString(contents, error_message);
}

std::optional<boost::json::value> LoadJsonFromString(
    std::string_view contents,
    std::string* error_message) {
  boost::system::error_code ec;
  boost::json::parse_options opts;
  opts.allow_trailing_commas = true;
  auto value = boost::json::parse(contents, ec, {}, opts);
  if (ec) {
    if (error_message)
      *error_message = ec.message();
    return std::nullopt;
  }
  return value;
}

bool SaveJsonToFile(const boost::json::value& data,
                    const std::filesystem::path& path) {
  std::ofstream ofs{path};
  if (!ofs)
    return false;
  ofs << boost::json::serialize(data);
  return ofs.good();
}

std::string SaveJsonToString(const boost::json::value& data) {
  return boost::json::serialize(data);
}
