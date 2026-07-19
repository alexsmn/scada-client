#include "user_access/user_access.h"

#include "scada/privileges.h"

#include <gtest/gtest.h>

#include <string_view>

namespace {

int Access(bool configure, bool control) {
  int bits = 0;
  if (configure)
    bits |= 1 << static_cast<int>(scada::Privilege::Configure);
  if (control)
    bits |= 1 << static_cast<int>(scada::Privilege::Control);
  return bits;
}

TEST(UserRoleForTest, DerivesRoleTiers) {
  EXPECT_EQ(UserRoleFor(Access(true, true)), UserRole::kAdministrator);
  EXPECT_EQ(UserRoleFor(Access(true, false)),
            UserRole::kAdministrator);  // Configure wins.
  EXPECT_EQ(UserRoleFor(Access(false, true)), UserRole::kOperator);
  EXPECT_EQ(UserRoleFor(Access(false, false)), UserRole::kObserver);
  EXPECT_EQ(std::string_view{UserRoleLabelKey(UserRole::kAdministrator)},
            "Administrator");
}

TEST(UserPermissionsForTest, ViewAlwaysGranted) {
  const auto perms = UserPermissionsFor(Access(false, false));
  ASSERT_EQ(perms.size(), 3u);
  EXPECT_EQ(perms[0].kind, UserPermissionKind::kView);
  EXPECT_TRUE(perms[0].granted);
}

TEST(UserPermissionsForTest, ControlAndConfigureTrackBits) {
  auto operator_perms = UserPermissionsFor(Access(false, true));
  EXPECT_TRUE(operator_perms[1].granted);   // Control
  EXPECT_FALSE(operator_perms[2].granted);  // Configure

  auto admin_perms = UserPermissionsFor(Access(true, true));
  EXPECT_TRUE(admin_perms[1].granted);
  EXPECT_TRUE(admin_perms[2].granted);

  auto observer_perms = UserPermissionsFor(Access(false, false));
  EXPECT_FALSE(observer_perms[1].granted);
  EXPECT_FALSE(observer_perms[2].granted);
}

}  // namespace
