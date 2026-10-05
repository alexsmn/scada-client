#include "user_access/user_access.h"

#include "model/namespaces.h"
#include "scada/authorization.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

// What the server's process map grants `role` (scada::ProcessRolePermissions),
// standing in for the Control object's RolePermissions read.
scada::Permission ProcessGrant(scada::WellKnownRole role) {
  for (const scada::RolePermissionType& entry :
       scada::ProcessRolePermissions()) {
    if (entry.role_id == scada::WellKnownRoleId(role)) {
      return entry.permissions;
    }
  }
  return scada::Permission::kNone;
}

// A Role as the SERVER published it: its id plus the grant that came back on
// the RolePermissions map. `DefaultPermissionsForRole` stands in for the
// server here — in the client it is only ever the wire that supplies this.
AccountRole Role(scada::WellKnownRole role, std::u16string name) {
  return AccountRole{scada::WellKnownRoleId(role), std::move(name),
                     scada::DefaultPermissionsForRole(role),
                     ProcessGrant(role)};
}

bool GrantedFor(const std::vector<UserPermission>& permissions,
                scada::Capability kind) {
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

  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kConfigure));
}

TEST(UserAccess, ObserverMayViewOnly) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kObserver, u"Observer")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kConfigure));
}

TEST(UserAccess, OperatorMayControlButNotConfigure) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kOperator, u"Operator")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kConfigure));
}

TEST(UserAccess, ConfigureAdminMayConfigure) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin")};

  EXPECT_TRUE(GrantedFor(PermissionsForRoles(roles),
                         scada::Capability::kConfigure));
}

// Several Roles union, they do not override — which is exactly why the
// inspector cannot show one derived tier.
TEST(UserAccess, MultipleRolesUnionTheirPermissions) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kObserver, u"Observer"),
      Role(scada::WellKnownRole::kOperator, u"Operator"),
      Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kConfigure));
}

// Backlog 864: the Configure right's Roles hold Write and Call on
// configuration, but not on process nodes, so an Engineer reads as
// configuring and NOT controlling — rights level 1, «Инженер АСУ и связи».
TEST(UserAccess, EngineerMayConfigureButNotControl) {
  const std::vector<AccountRole> roles = {
      Role(scada::WellKnownRole::kObserver, u"Observer"),
      Role(scada::WellKnownRole::kEngineer, u"Engineer"),
      Role(scada::WellKnownRole::kSupervisor, u"Supervisor"),
      Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin"),
      Role(scada::WellKnownRole::kSecurityAdmin, u"SecurityAdmin")};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_TRUE(GrantedFor(permissions, scada::Capability::kConfigure));
}

// A custom (group) Role's permissions are a per-namespace policy the client
// cannot read (Part 3 §5.2.9), so the server publishes no grant for it and it
// contributes nothing rather than being guessed at.
TEST(UserAccess, CustomRoleContributesNothing) {
  const std::vector<AccountRole> roles = {
      AccountRole{scada::NodeId{7, scada::NamespaceIndexes::ROLE}, u"Shift A",
                  std::nullopt}};

  const auto permissions = PermissionsForRoles(roles);

  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kView));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kControl));
  EXPECT_FALSE(GrantedFor(permissions, scada::Capability::kConfigure));
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
