#include "base/secret_store.h"

#include "base/protected_settings_secret_store.h"
#include "base/settings_store.h"

// wincrypt.h before dpapi.h, explicitly: windows.h only pulls it in when
// WIN32_LEAN_AND_MEAN is not defined, and dpapi.h does not include it.
#include <windows.h>

#include <wincrypt.h>

#include <dpapi.h>

namespace {

// Mixed into every blob, so a value protected by some other program for the
// same user does not unprotect here by accident. Not a secret.
constexpr char kEntropy[] = "Telecontrol SCADA Client secret";

DATA_BLOB MakeBlob(std::string_view bytes) {
  return {static_cast<DWORD>(bytes.size()),
          reinterpret_cast<BYTE*>(const_cast<char*>(bytes.data()))};
}

// DPAPI with the current user's key: only this Windows account, on this
// machine (or its roaming profile), can unprotect it. CryptProtectData,
// https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata
std::optional<std::string> Transform(std::string_view input, bool protect) {
  DATA_BLOB in = MakeBlob(input);
  DATA_BLOB entropy = MakeBlob(kEntropy);
  DATA_BLOB out = {};
  const BOOL ok =
      protect ? ::CryptProtectData(&in, nullptr, &entropy, nullptr, nullptr,
                                   CRYPTPROTECT_UI_FORBIDDEN, &out)
              : ::CryptUnprotectData(&in, nullptr, &entropy, nullptr, nullptr,
                                     CRYPTPROTECT_UI_FORBIDDEN, &out);
  if (!ok)
    return std::nullopt;
  std::string result{reinterpret_cast<const char*>(out.pbData), out.cbData};
  ::SecureZeroMemory(out.pbData, out.cbData);
  ::LocalFree(out.pbData);
  return result;
}

}  // namespace

std::shared_ptr<SecretStore> CreatePlatformSecretStore(
    std::shared_ptr<SettingsStore> settings_store) {
  return std::make_shared<ProtectedSettingsSecretStore>(
      std::move(settings_store),
      [](std::string_view plain) { return Transform(plain, true); },
      [](std::string_view blob) { return Transform(blob, false); });
}
