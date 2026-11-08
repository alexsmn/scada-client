#pragma once

#include "base/secret_store.h"
#include "base/settings_store.h"

#include <memory>

// A SecretStore that keeps secrets UNPROTECTED, as plain values of the same
// name in a SettingsStore. Never the operator's store: it exists for the
// harnesses that hand the Client a settings store of their own -- the E2E
// suite writes `Password` into its test-settings file and relaunches against
// it (docs/ops/e2e-client-server.md), the screenshot generator uses a memory
// store -- so they keep working unchanged and never reach the real Keychain
// or DPAPI-protected value.
class PlainSettingsSecretStore final : public SecretStore {
 public:
  explicit PlainSettingsSecretStore(
      std::shared_ptr<SettingsStore> settings_store)
      : settings_store_{std::move(settings_store)} {}

  std::optional<std::u16string> Read(std::string_view name) const override {
    std::u16string value = settings_store_->ReadString16(name);
    if (value.empty())
      return std::nullopt;
    return value;
  }

  bool Write(std::string_view name, std::u16string_view value) override {
    return settings_store_->Write(name, value);
  }

  bool Erase(std::string_view name) override {
    return settings_store_->Erase(name);
  }

 private:
  const std::shared_ptr<SettingsStore> settings_store_;
};
