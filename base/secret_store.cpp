#include "base/secret_store.h"

// Windows and macOS define CreatePlatformSecretStore in secret_store_win.cpp
// and secret_store_mac.cpp. Everywhere else -- Linux, where the Client is not
// shipped -- there is no per-user secret facility we can count on (libsecret
// needs a running Secret Service and is not a dependency), so the fallback
// persists nothing: an automatic login there asks for the password again
// rather than leaving it in the clear.
#if !defined(_WIN32) && !defined(__APPLE__)

namespace {

class UnavailableSecretStore final : public SecretStore {
 public:
  std::optional<std::u16string> Read(std::string_view) const override {
    return std::nullopt;
  }
  bool Write(std::string_view, std::u16string_view) override { return false; }
  bool Erase(std::string_view) override { return true; }
};

}  // namespace

std::shared_ptr<SecretStore> CreatePlatformSecretStore(
    std::shared_ptr<SettingsStore> /*settings_store*/) {
  return std::make_shared<UnavailableSecretStore>();
}

#endif
