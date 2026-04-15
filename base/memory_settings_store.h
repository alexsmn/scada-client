#pragma once

#include "base/settings_store.h"

#include <map>
#include <optional>
#include <string>

class MemorySettingsStore final : public SettingsStore {
 public:
  void Clear() {
    bool_values_.clear();
    string_values_.clear();
    string16_values_.clear();
  }

  void SetBool(std::string_view name, bool value) {
    bool_values_[std::string{name}] = value;
  }

  void SetString(std::string_view name, std::string_view value) {
    string_values_[std::string{name}] = std::string{value};
  }

  void SetString16(std::string_view name, std::u16string_view value) {
    string16_values_[std::string{name}] = std::u16string{value};
  }

  bool HasBool(std::string_view name) const {
    return bool_values_.contains(std::string{name});
  }

  bool HasString(std::string_view name) const {
    return string_values_.contains(std::string{name});
  }

  bool HasString16(std::string_view name) const {
    return string16_values_.contains(std::string{name});
  }

  std::optional<bool> GetBool(std::string_view name) const {
    if (auto it = bool_values_.find(std::string{name}); it != bool_values_.end())
      return it->second;
    return std::nullopt;
  }

  std::optional<std::string> GetString(std::string_view name) const {
    if (auto it = string_values_.find(std::string{name});
        it != string_values_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  std::optional<std::u16string> GetString16(std::string_view name) const {
    if (auto it = string16_values_.find(std::string{name});
        it != string16_values_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  bool ReadBool(std::string_view name) const override {
    if (auto it = bool_values_.find(std::string{name});
        it != bool_values_.end()) {
      return it->second;
    }
    return false;
  }

  std::string ReadString(std::string_view name) const override {
    if (auto it = string_values_.find(std::string{name});
        it != string_values_.end()) {
      return it->second;
    }
    return {};
  }

  std::u16string ReadString16(std::string_view name) const override {
    if (auto it = string16_values_.find(std::string{name});
        it != string16_values_.end()) {
      return it->second;
    }
    return {};
  }

  bool Write(std::string_view name, bool bool_value) override {
    SetBool(name, bool_value);
    return true;
  }

  bool Write(std::string_view name, std::string_view string_value) override {
    SetString(name, string_value);
    return true;
  }

  bool Write(std::string_view name,
             std::u16string_view string16_value) override {
    SetString16(name, string16_value);
    return true;
  }

 private:
  std::map<std::string, bool> bool_values_;
  std::map<std::string, std::string> string_values_;
  std::map<std::string, std::u16string> string16_values_;
};
