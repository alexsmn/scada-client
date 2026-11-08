#include "base/protected_settings_secret_store.h"

#include "base/base64.h"
#include "base/settings_store.h"
#include "base/utf_convert.h"

ProtectedSettingsSecretStore::ProtectedSettingsSecretStore(
    std::shared_ptr<SettingsStore> settings_store,
    Protect protect,
    Unprotect unprotect)
    : settings_store_{std::move(settings_store)},
      protect_{std::move(protect)},
      unprotect_{std::move(unprotect)} {}

// static
std::string ProtectedSettingsSecretStore::ValueName(std::string_view name) {
  return std::string{name} + "Protected";
}

std::optional<std::u16string> ProtectedSettingsSecretStore::Read(
    std::string_view name) const {
  const std::string encoded = settings_store_->ReadString(ValueName(name));
  if (encoded.empty())
    return std::nullopt;

  std::string blob;
  scada::base::Base64Decode(encoded, &blob);
  if (blob.empty())
    return std::nullopt;

  std::optional<std::string> plain = unprotect_(blob);
  if (!plain)
    return std::nullopt;
  return UtfConvert<char16_t>(*plain);
}

bool ProtectedSettingsSecretStore::Write(std::string_view name,
                                         std::u16string_view value) {
  std::optional<std::string> blob = protect_(UtfConvert<char>(value));
  if (!blob || blob->empty())
    return false;

  std::string encoded;
  scada::base::Base64Encode(*blob, &encoded);
  return settings_store_->Write(ValueName(name), std::string_view{encoded});
}

bool ProtectedSettingsSecretStore::Erase(std::string_view name) {
  return settings_store_->Erase(ValueName(name));
}
