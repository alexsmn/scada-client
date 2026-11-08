#pragma once

#include "base/secret_store.h"

#include <map>
#include <string>

// An in-process SecretStore: holds secrets for the lifetime of the object and
// persists nothing. For unit tests.
class MemorySecretStore final : public SecretStore {
 public:
  std::optional<std::u16string> Read(std::string_view name) const override {
    if (auto it = values_.find(std::string{name}); it != values_.end())
      return it->second;
    return std::nullopt;
  }

  bool Write(std::string_view name, std::u16string_view value) override {
    if (!writable_)
      return false;
    values_[std::string{name}] = std::u16string{value};
    return true;
  }

  bool Erase(std::string_view name) override {
    values_.erase(std::string{name});
    return true;
  }

  // Makes Write fail, as a store with no protection available does.
  void set_writable(bool writable) { writable_ = writable; }

 private:
  std::map<std::string, std::u16string> values_;
  bool writable_ = true;
};
