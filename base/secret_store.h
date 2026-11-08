#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

class SettingsStore;

// Keeps secrets -- today only the automatic-login password -- out of the
// plain settings store, which is a REG_SZ under HKCU on Windows and a JSON
// file on macOS that anyone with the user profile or a backup can read
// (backlog 1030). Implementations protect what they hold with the platform's
// per-user secret facility; see CreatePlatformSecretStore().
class SecretStore {
 public:
  virtual ~SecretStore() = default;

  // The secret stored under `name`, or nullopt when there is none or it can
  // no longer be recovered (e.g. it was protected for another Windows user).
  virtual std::optional<std::u16string> Read(std::string_view name) const = 0;

  // Stores `value` under `name`, replacing what was there. Returns false when
  // the secret could not be stored protected; nothing is then written in the
  // clear instead.
  virtual bool Write(std::string_view name, std::u16string_view value) = 0;

  // Removes the secret `name`. Returns true when it is gone afterwards,
  // including when it was never there.
  virtual bool Erase(std::string_view name) = 0;
};

// The operator's real secret store for this platform:
//  - Windows: DPAPI (CryptProtectData, current-user scope), the protected
//    blob kept base64-encoded in `settings_store`
//    (ProtectedSettingsSecretStore).
//  - macOS: a generic-password item in the login Keychain.
//  - Anything else: nothing is persisted. The Client is not shipped for
//    Linux and there is no Security-framework equivalent everywhere, so Write
//    fails rather than fall back to the clear text this store exists to
//    replace.
std::shared_ptr<SecretStore> CreatePlatformSecretStore(
    std::shared_ptr<SettingsStore> settings_store);

#if defined(__APPLE__)
// A Keychain-backed store whose items carry `service` as kSecAttrService and
// the secret's name as kSecAttrAccount. Exposed so a test can use a service
// name of its own and never touch the Client's real item.
std::shared_ptr<SecretStore> CreateKeychainSecretStore(std::string service);
#endif
