#include "base/file_settings_store.h"

#include "base/utf_convert.h"

#include <boost/json.hpp>
#include <boost/system/error_code.hpp>
#include <fstream>

FileSettingsStore::FileSettingsStore(std::filesystem::path path)
    : path_{std::move(path)} {
  Load();
}

bool FileSettingsStore::ReadBool(std::string_view name) const {
  if (auto it = bool_values_.find(std::string{name}); it != bool_values_.end())
    return it->second;
  return false;
}

std::string FileSettingsStore::ReadString(std::string_view name) const {
  if (auto it = string_values_.find(std::string{name});
      it != string_values_.end()) {
    return it->second;
  }
  return {};
}

std::u16string FileSettingsStore::ReadString16(std::string_view name) const {
  return UtfConvert<char16_t>(ReadString(name));
}

bool FileSettingsStore::Write(std::string_view name, bool bool_value) {
  bool_values_[std::string{name}] = bool_value;
  string_values_.erase(std::string{name});
  return Save();
}

bool FileSettingsStore::Write(std::string_view name,
                              std::string_view string_value) {
  string_values_[std::string{name}] = std::string{string_value};
  bool_values_.erase(std::string{name});
  return Save();
}

bool FileSettingsStore::Write(std::string_view name,
                              std::u16string_view string16_value) {
  return Write(name, UtfConvert<char>(string16_value));
}

void FileSettingsStore::Load() {
  bool_values_.clear();
  string_values_.clear();

  std::ifstream input{path_};
  if (!input)
    return;

  std::string content{std::istreambuf_iterator<char>{input},
                      std::istreambuf_iterator<char>{}};
  if (content.empty())
    return;

  boost::system::error_code error;
  auto value = boost::json::parse(content, error);
  if (error || !value.is_object())
    return;

  for (auto it = value.as_object().begin(); it != value.as_object().end();
       ++it) {
    const std::string name = it->key_c_str();
    const auto& json_value = it->value();
    if (json_value.is_bool()) {
      bool_values_[std::string{name}] = json_value.as_bool();
    } else if (json_value.is_string()) {
      string_values_[std::string{name}] =
          std::string{json_value.as_string().c_str()};
    }
  }
}

bool FileSettingsStore::Save() const {
  boost::json::object root;
  for (const auto& [name, value] : bool_values_)
    root[name] = value;
  for (const auto& [name, value] : string_values_)
    root[name] = value;

  std::error_code ec;
  if (path_.has_parent_path())
    std::filesystem::create_directories(path_.parent_path(), ec);

  std::ofstream output{path_, std::ios::binary | std::ios::trunc};
  if (!output)
    return false;

  output << boost::json::serialize(root);
  return output.good();
}
