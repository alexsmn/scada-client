#include "user_access/user_access.h"

#include "model/namespaces.h"
#include "scada/authorization.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

AccountRole Role(scada::WellKnownRole role, std::u16string name) {
  return AccountRole{scada::WellKnownRoleId(role), std::move(name)};
}

bool GrantedFor(const std::vector<UserPermission>& permissions,
                UserPermissionKind kind) {
  for (const UserPermission& permission : permissions) {
    if (permission.kind == kind) {
      return permission.granted;
    }
  }
  ADD_FAILURE() << "permission kind missing from the breakdown";
  return false;
}

// An account with no Role gets nothing. This is the point of the cutover:
// rights come from Roles, so holding none means holding nothing — not the
// "Observer, view only" tier a zero access-rights bitmask used to imply.
TEST(UserAccess, NoRolesGrantsNothing) {
  const auto permissions = PermissionsForRoles({});

  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kView));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kControl));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kConfigure));
}

TEST(UserAccess, ObserverMayViewOnly) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kObserver, u"Observer")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kView));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kControl));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kConfigure));
}

TEST(UserAccess, OperatorMayControlButNotConfigure) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kOperator, u"Operator")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kView));
  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kControl));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kConfigure));
}

TEST(UserAccess, ConfigureAdminMayConfigure) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin")};

  EXPECT_TRUE(GrantedFor(PermissionsForRoles(roles),
                         UserPermissionKind::kConfigure));
}

// Several Roles union, they do not override — which is exactly why the
// inspector cannot show one derived tier.
TEST(UserAccess, MultipleRolesUnionTheirPermissions) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kObserver, u"Observer"),
      Role(scada::WellKnownRole::kOperator, u"Operator"),
      Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kView));
  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kControl));
  EXPECT_TRUE(GrantedFor(permissions, UserPermissionKind::kConfigure));
}

// A custom (group) Role's permissions are a per-namespace policy the client
// cannot read (Part 3 §5.2.9), so it contributes nothing rather than being
// guessed at.
TEST(UserAccess, CustomRoleContributesNothing) {
  const std::vector<AccountRole> roles = {
      AccountRole{scada::NodeId{7, scada::NamespaceIndexes::ROLE}, u"Shift A"}};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kView));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kControl));
  EXPECT_FALSE(GrantedFor(permissions, UserPermissionKind::kConfigure));
}

// The client's coarse capabilities are backed by the same map the server
// enforces with, so a row cannot claim what the server would refuse.
TEST(UserAccess, EffectivePermissionsMatchTheServerRoleMap) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kOperator, u"Operator")};

  EXPECT_EQ(EffectivePermissions(roles),
            scada::DefaultPermissionsForRole(scada::WellKnownRole::kOperator));
}

TEST(UserAccess, SessionsLabelDistinguishesUnknownFromSingle) {
  EXPECT_STREQ(UserSessionsLabelKey(std::nullopt), "No data");
  EXPECT_STREQ(UserSessionsLabelKey(false), "Single");
  EXPECT_STREQ(UserSessionsLabelKey(true), "Multiple");
}

}  // namespace
