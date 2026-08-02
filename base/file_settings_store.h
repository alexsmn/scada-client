#pragma once

#include "base/settings_store.h"

#include <filesystem>
#include <map>

class FileSettingsStore final : public SettingsStore {
 public:
  explicit FileSettingsStore(std::filesystem::path path);

  bool ReadBool(std::string_view name) const override;
  std::string ReadString(std::string_view name) const override;
  std::u16string ReadString16(std::string_view name) const override;

  bool Write(std::string_view name, bool bool_value) override;
  bool Write(std::string_view name, std::string_view string_value) override;
  bool Write(std::string_view name,
             std::u16string_view string16_value) override;

 private:
  void Load();
  bool Save() const;

  std::filesystem::path path_;
  std::map<std::string, bool> bool_values_;
  std::map<std::string, std::string> string_values_;
};
