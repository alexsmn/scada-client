#pragma once

#include "base/secret_store.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class SettingsStore;

// A SecretStore that keeps each secret as an opaque protected blob,
// base64-encoded, in an ordinary SettingsStore under "<name>Protected". The
// protection itself is the pair of functions it is given: on Windows they are
// DPAPI's CryptProtectData / CryptUnprotectData. Keeping the encoding and the
// storage here, apart from the platform call, lets everything but the two
// DPAPI calls be tested on any host.
class ProtectedSettingsSecretStore final : public SecretStore {
 public:
  // Turns plain bytes into a protected blob, or nullopt on failure.
  using Protect = std::function<std::optional<std::string>(std::string_view)>;
  // Turns a protected blob back into plain bytes, or nullopt on failure.
  using Unprotect = std::function<std::optional<std::string>(std::string_view)>;

  ProtectedSettingsSecretStore(std::shared_ptr<SettingsStore> settings_store,
                               Protect protect,
                               Unprotect unprotect);

  std::optional<std::u16string> Read(std::string_view name) const override;
  bool Write(std::string_view name, std::u16string_view value) override;
  bool Erase(std::string_view name) override;

  // The settings value a secret named `name` is kept under.
  static std::string ValueName(std::string_view name);

 private:
  const std::shared_ptr<SettingsStore> settings_store_;
  const Protect protect_;
  const Unprotect unprotect_;
};
