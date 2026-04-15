#pragma once

#include <windows.h>

#include <memory>
#include <string>
#include <string_view>

class SettingsStore {
 public:
  virtual ~SettingsStore() = default;

  virtual bool ReadBool(std::string_view name) const = 0;
  virtual std::string ReadString(std::string_view name) const = 0;
  virtual std::u16string ReadString16(std::string_view name) const = 0;

  virtual bool Write(std::string_view name, bool bool_value) = 0;
  virtual bool Write(std::string_view name, std::string_view string_value) = 0;
  virtual bool Write(std::string_view name,
                     std::u16string_view string16_value) = 0;
};

class RegistrySettingsStore final : public SettingsStore {
 public:
  RegistrySettingsStore(HKEY root, std::wstring subkey);

  bool ReadBool(std::string_view name) const override;
  std::string ReadString(std::string_view name) const override;
  std::u16string ReadString16(std::string_view name) const override;

  bool Write(std::string_view name, bool bool_value) override;
  bool Write(std::string_view name, std::string_view string_value) override;
  bool Write(std::string_view name,
             std::u16string_view string16_value) override;

 private:
  HKEY root_;
  std::wstring subkey_;
};
