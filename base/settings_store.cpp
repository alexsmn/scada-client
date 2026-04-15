#include "base/settings_store.h"

#include "base/utf_convert.h"
#include "base/win/registry.h"

namespace {

class RegHelper {
 public:
  explicit RegHelper(base::win::RegKey& key) : key_{key} {}

  bool ReadBool(std::string_view name) const {
    DWORD value = 0;
    key_.ReadValueDW(UtfConvert<wchar_t>(name).c_str(), &value);
    return value != 0;
  }

  std::string ReadString(std::string_view name) const {
    std::wstring value;
    key_.ReadValue(UtfConvert<wchar_t>(name).c_str(), &value);
    return UtfConvert<char>(value);
  }

  std::u16string ReadString16(std::string_view name) const {
    std::wstring value;
    key_.ReadValue(UtfConvert<wchar_t>(name).c_str(), &value);
    return UtfConvert<char16_t>(value);
  }

  bool Write(std::string_view name, bool bool_value) {
    DWORD dword_value = bool_value ? 1 : 0;
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(), dword_value) ==
           ERROR_SUCCESS;
  }

  bool Write(std::string_view name, std::string_view string_value) {
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(),
                           UtfConvert<wchar_t>(string_value).c_str()) ==
           ERROR_SUCCESS;
  }

  bool Write(std::string_view name, std::u16string_view string16_value) {
    return key_.WriteValue(UtfConvert<wchar_t>(name).c_str(),
                           UtfConvert<wchar_t>(string16_value).c_str()) ==
           ERROR_SUCCESS;
  }

 private:
  base::win::RegKey& key_;
};

}  // namespace

RegistrySettingsStore::RegistrySettingsStore(HKEY root, std::wstring subkey)
    : root_{root}, subkey_{std::move(subkey)} {}

bool RegistrySettingsStore::ReadBool(std::string_view name) const {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_QUERY_VALUE);
  return RegHelper{reg}.ReadBool(name);
}

std::string RegistrySettingsStore::ReadString(std::string_view name) const {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_QUERY_VALUE);
  return RegHelper{reg}.ReadString(name);
}

std::u16string RegistrySettingsStore::ReadString16(
    std::string_view name) const {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_QUERY_VALUE);
  return RegHelper{reg}.ReadString16(name);
}

bool RegistrySettingsStore::Write(std::string_view name, bool bool_value) {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_SET_VALUE | KEY_QUERY_VALUE);
  return RegHelper{reg}.Write(name, bool_value);
}

bool RegistrySettingsStore::Write(std::string_view name,
                                  std::string_view string_value) {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_SET_VALUE | KEY_QUERY_VALUE);
  return RegHelper{reg}.Write(name, string_value);
}

bool RegistrySettingsStore::Write(std::string_view name,
                                  std::u16string_view string16_value) {
  base::win::RegKey reg(root_, subkey_.c_str(), KEY_SET_VALUE | KEY_QUERY_VALUE);
  return RegHelper{reg}.Write(name, string16_value);
}
