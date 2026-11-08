#include "base/protected_settings_secret_store.h"

#include "base/base64.h"
#include "base/memory_settings_store.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace {

// Stands in for DPAPI: a reversible transform with a marker, so a test can
// tell a protected blob from the plain bytes and make either side fail.
struct FakeProtector {
  bool fail_protect = false;
  bool fail_unprotect = false;

  std::optional<std::string> Protect(std::string_view plain) const {
    if (fail_protect)
      return std::nullopt;
    std::string blob{"P:"};
    blob.append(plain.rbegin(), plain.rend());
    return blob;
  }

  std::optional<std::string> Unprotect(std::string_view blob) const {
    if (fail_unprotect || !blob.starts_with("P:"))
      return std::nullopt;
    blob.remove_prefix(2);
    return std::string{blob.rbegin(), blob.rend()};
  }
};

class ProtectedSettingsSecretStoreTest : public testing::Test {
 protected:
  std::shared_ptr<MemorySettingsStore> settings_ =
      std::make_shared<MemorySettingsStore>();
  FakeProtector protector_;
  ProtectedSettingsSecretStore store_{
      settings_,
      [this](std::string_view plain) { return protector_.Protect(plain); },
      [this](std::string_view blob) { return protector_.Unprotect(blob); }};
};

TEST_F(ProtectedSettingsSecretStoreTest, RoundTripsThroughAProtectedValue) {
  ASSERT_TRUE(store_.Write("Password", u"secret-1"));

  EXPECT_EQ(store_.Read("Password"),
            std::optional<std::u16string>{u"secret-1"});
  // Only the protected blob, base64-encoded, reaches the settings, under its
  // own name; the plain name is not written.
  EXPECT_EQ(settings_->GetString16("Password"), std::nullopt);
  const std::optional<std::string> stored =
      settings_->GetString(ProtectedSettingsSecretStore::ValueName("Password"));
  ASSERT_TRUE(stored.has_value());
  std::string blob;
  scada::base::Base64Decode(*stored, &blob);
  EXPECT_EQ(blob, "P:1-terces");
}

// Non-ASCII passwords survive the UTF-8 round trip.
TEST_F(ProtectedSettingsSecretStoreTest, RoundTripsNonAsciiText) {
  ASSERT_TRUE(
      store_.Write("Password", u"\u043f\u0430\u0440\u043e\u043b\u044c"));
  EXPECT_EQ(
      store_.Read("Password"),
      std::optional<std::u16string>{u"\u043f\u0430\u0440\u043e\u043b\u044c"});
}

TEST_F(ProtectedSettingsSecretStoreTest, MissingSecretReadsAsNothing) {
  EXPECT_EQ(store_.Read("Password"), std::nullopt);
}

TEST_F(ProtectedSettingsSecretStoreTest, FailedProtectionWritesNothing) {
  protector_.fail_protect = true;

  EXPECT_FALSE(store_.Write("Password", u"secret"));
  EXPECT_EQ(
      settings_->GetString(ProtectedSettingsSecretStore::ValueName("Password")),
      std::nullopt);
}

// A blob protected for another Windows user (a copied profile, a restored
// backup) does not unprotect; it reads as no secret rather than as garbage.
TEST_F(ProtectedSettingsSecretStoreTest, UnrecoverableBlobReadsAsNothing) {
  ASSERT_TRUE(store_.Write("Password", u"secret"));
  protector_.fail_unprotect = true;

  EXPECT_EQ(store_.Read("Password"), std::nullopt);
}

TEST_F(ProtectedSettingsSecretStoreTest, EraseRemovesTheValue) {
  ASSERT_TRUE(store_.Write("Password", u"secret"));

  EXPECT_TRUE(store_.Erase("Password"));
  EXPECT_EQ(store_.Read("Password"), std::nullopt);
  EXPECT_EQ(
      settings_->GetString(ProtectedSettingsSecretStore::ValueName("Password")),
      std::nullopt);
  EXPECT_TRUE(store_.Erase("Password")) << "erasing nothing succeeds";
}

}  // namespace
