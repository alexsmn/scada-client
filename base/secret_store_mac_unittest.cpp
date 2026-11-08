#include "base/secret_store.h"

#if defined(__APPLE__)

#include <gtest/gtest.h>

#include <unistd.h>

#include <format>

namespace {

// Exercises the real login Keychain under a service name of the test's own,
// distinct per process, and removes its item afterwards. It never touches the
// Client's item ("Telecontrol SCADA Client").
class KeychainSecretStoreTest : public testing::Test {
 protected:
  void TearDown() override { store_->Erase(kName); }

  static constexpr char kName[] = "Password";
  std::shared_ptr<SecretStore> store_ = CreateKeychainSecretStore(
      std::format("Telecontrol SCADA Client unittest {}", ::getpid()));
};

TEST_F(KeychainSecretStoreTest, WritesReadsReplacesAndErases) {
  if (!store_->Write(kName, u"\u043f\u0435\u0440\u0432\u044b\u0439")) {
    // A locked keychain with no UI to unlock it (an ssh session, a CI runner)
    // refuses with errSecInteractionNotAllowed. That is the environment, not
    // the store.
    GTEST_SKIP() << "the login keychain is not writable in this session";
  }
  EXPECT_EQ(store_->Read(kName), std::optional<std::u16string>{
                                     u"\u043f\u0435\u0440\u0432\u044b\u0439"});

  ASSERT_TRUE(store_->Write(kName, u"second"));
  EXPECT_EQ(store_->Read(kName), std::optional<std::u16string>{u"second"});

  EXPECT_TRUE(store_->Erase(kName));
  EXPECT_EQ(store_->Read(kName), std::nullopt);
  EXPECT_TRUE(store_->Erase(kName)) << "erasing nothing succeeds";
}

}  // namespace

#endif  // defined(__APPLE__)
