#include "user_access/role_membership.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/test/fake_node_service.h"
#include "scada/authorization.h"
#include "scada/identity_mapping_rule_encoding.h"
#include "scada/method_service.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"
#include "user_access/test/fake_role_permissions_service.h"

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

namespace {

const scada::NodeId kRoleSet{scada::id::Server_ServerCapabilities_RoleSet,
                             scada::NamespaceIndexes::NS0};

class RoleMembershipTest : public ::testing::Test {
 protected:
  RoleMembershipTest() {
    node_service_.Add(scada::NodeState{
        .node_id = scada::security::id::IdentityMappingRuleType,
        .node_class = scada::NodeClass::ObjectType});
  }

  void AddRoleSet() {
    node_service_.Add(scada::NodeState{.node_id = kRoleSet,
                                       .node_class = scada::NodeClass::Object});
  }

  void AddRole(const scada::NodeId& role_id, std::u16string name) {
    node_service_.Add(
        scada::NodeState{.node_id = role_id,
                         .node_class = scada::NodeClass::Object,
                         .parent_id = kRoleSet,
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.display_name = std::move(name)}});
  }

  void AddRule(const scada::NodeId& role_id,
               scada::UInt32 rule_id,
               const std::string& criteria,
               scada::IdentityCriteriaType criteria_type =
                   scada::IdentityCriteriaType::kUserName) {
    node_service_.Add(scada::NodeState{
        .node_id = scada::NodeId{rule_id, 31},
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::security::id::IdentityMappingRuleType,
        .parent_id = role_id,
        .reference_type_id = scada::id::Organizes,
        .properties = {
            {scada::security::id::IdentityMappingRuleType_CriteriaType,
             static_cast<scada::Int32>(criteria_type)},
            {scada::security::id::IdentityMappingRuleType_Criteria,
             criteria}}});
  }

  std::optional<std::vector<RoleMembership>> Read() {
    return RunAwaitable(
        io_, [this]() -> Awaitable<std::optional<std::vector<RoleMembership>>> {
          co_return co_await ReadRoleMemberships(
              io_.get_executor(), node_service_, attribute_service_);
        });
  }

  boost::asio::io_context io_;
  FakeNodeService node_service_;
  FakeRolePermissionsService attribute_service_;
};

TEST_F(RoleMembershipTest, ReadsEachRoleAndItsMembers) {
  AddRoleSet();
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  AddRole(op, u"Operator");
  AddRule(op, 1, "ivanov");
  AddRule(op, 2, "petrov");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_EQ((*roles)[0].name, u"Operator");
  EXPECT_EQ((*roles)[0].members,
            (std::vector<std::u16string>{u"ivanov", u"petrov"}));
}

// A Role nobody holds is a real state — an empty Role is how one starts.
TEST_F(RoleMembershipTest, ARoleWithNoRulesHasNoMembers) {
  AddRoleSet();
  AddRole(scada::WellKnownRoleId(scada::WellKnownRole::kEngineer), u"Engineer");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_TRUE((*roles)[0].members.empty());
}

// The well-known Roles' NODES cannot be deleted, which is the one structural
// fact the view reports about a Role.
TEST_F(RoleMembershipTest, MarksWellKnownRoles) {
  AddRoleSet();
  AddRole(scada::WellKnownRoleId(scada::WellKnownRole::kOperator), u"Operator");
  AddRole(scada::NodeId{7, scada::NamespaceIndexes::ROLE}, u"Shift A");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 2u);
  EXPECT_TRUE((*roles)[0].well_known);
  EXPECT_FALSE((*roles)[1].well_known);
}

// Only UserName criteria name an account; the other IdentityCriteriaType
// values describe token classes a username/password account never presents.
TEST_F(RoleMembershipTest, IgnoresNonUserNameCriteria) {
  AddRoleSet();
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  AddRole(op, u"Operator");
  AddRule(op, 1, "ivanov", scada::IdentityCriteriaType::kThumbprint);

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_TRUE((*roles)[0].members.empty());
}

// A conformant server always publishes the well-known Roles, so an empty
// browse is a failed read, not a server without Roles.
TEST_F(RoleMembershipTest, AbsentRoleSetIsUnknownNotEmpty) {
  EXPECT_FALSE(Read().has_value());
}

TEST_F(RoleMembershipTest, EmptyRoleSetIsUnknownNotEmpty) {
  AddRoleSet();

  EXPECT_FALSE(Read().has_value());
}

// Each Role carries what the SERVER says it grants, read off the published
// RolePermissions map. This is what lets the client drop its own copy of that
// map — and therefore what stops it stating something the server contradicts.
TEST_F(RoleMembershipTest, RolesCarryTheServerPublishedGrant) {
  AddRoleSet();
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  AddRole(op, u"Operator");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_EQ((*roles)[0].permissions,
            scada::DefaultPermissionsForRole(scada::WellKnownRole::kOperator));
}

// Backlog 864: an Engineer holds Write and Call on configuration but not on
// process nodes, and the second grant is read off the server's process map.
TEST_F(RoleMembershipTest, RolesCarryTheServerPublishedProcessGrant) {
  AddRoleSet();
  const scada::NodeId engineer =
      scada::WellKnownRoleId(scada::WellKnownRole::kEngineer);
  AddRole(engineer, u"Engineer");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  ASSERT_TRUE((*roles)[0].permissions.has_value());
  ASSERT_TRUE((*roles)[0].process_permissions.has_value());
  EXPECT_TRUE(
      scada::Contains(*(*roles)[0].permissions, scada::Permission::kWrite));
  EXPECT_FALSE(scada::Contains(*(*roles)[0].process_permissions,
                               scada::Permission::kWrite));
  EXPECT_FALSE(scada::Contains(*(*roles)[0].process_permissions,
                               scada::Permission::kCall));
}

// A server that predates the process map enforced the default map on process
// nodes too, so the default grant stands in for it rather than "nothing".
TEST(RoleMembershipProcessMapUnreadable, FallsBackToTheDefaultGrant) {
  boost::asio::io_context io;
  FakeNodeService node_service;
  FakeRolePermissionsService attribute_service{scada::DefaultRolePermissions()};

  node_service.Add(
      scada::NodeState{.node_id = scada::security::id::IdentityMappingRuleType,
                       .node_class = scada::NodeClass::ObjectType});
  node_service.Add(scada::NodeState{.node_id = kRoleSet,
                                    .node_class = scada::NodeClass::Object});
  node_service.Add(scada::NodeState{
      .node_id = scada::WellKnownRoleId(scada::WellKnownRole::kEngineer),
      .node_class = scada::NodeClass::Object,
      .parent_id = kRoleSet,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.display_name = u"Engineer"}});

  const auto roles = RunAwaitable(
      io, [&]() -> Awaitable<std::optional<std::vector<RoleMembership>>> {
        co_return co_await ReadRoleMemberships(io.get_executor(), node_service,
                                               attribute_service);
      });

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_EQ((*roles)[0].process_permissions, (*roles)[0].permissions);
}

// A custom (group) Role has no entry in the published map — its grants are a
// per-namespace policy (Part 3 §5.2.9) — so it carries none, rather than a
// guess.
TEST_F(RoleMembershipTest, CustomRoleCarriesNoGrant) {
  AddRoleSet();
  AddRole(scada::NodeId{7, scada::NamespaceIndexes::ROLE}, u"Shift A");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_FALSE((*roles)[0].permissions.has_value());
}

// Without the server's map there is nothing truthful to say about what any
// Role means, so the whole read reports unknown. Falling back to an assumed
// map is precisely the failure this design removes.
TEST(RoleMembershipMapUnreadable, UnreadableRoleMapIsUnknownNotAssumed) {
  boost::asio::io_context io;
  FakeNodeService node_service;
  FakeRolePermissionsService attribute_service{std::nullopt};

  node_service.Add(scada::NodeState{.node_id = kRoleSet,
                                    .node_class = scada::NodeClass::Object});
  node_service.Add(scada::NodeState{
      .node_id = scada::WellKnownRoleId(scada::WellKnownRole::kOperator),
      .node_class = scada::NodeClass::Object,
      .parent_id = kRoleSet,
      .reference_type_id = scada::id::Organizes,
      .attributes = {.display_name = u"Operator"}});

  const auto roles = RunAwaitable(
      io, [&]() -> Awaitable<std::optional<std::vector<RoleMembership>>> {
        co_return co_await ReadRoleMemberships(io.get_executor(), node_service,
                                               attribute_service);
      });

  EXPECT_FALSE(roles.has_value());
}

// The per-user inversion both the Users grid and the RBAC inspector consume.
// Each held Role carries its ID as well as its name: the name is what an
// operator reads, the id is what the effective permissions are derived from.
TEST_F(RoleMembershipTest, InvertsMembershipByAccount) {
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  const scada::NodeId admin =
      scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);
  const std::vector<RoleMembership> roles = {
      {.node_id = op, .name = u"Operator", .members = {u"ivanov", u"petrov"}},
      {.node_id = admin, .name = u"ConfigureAdmin", .members = {u"ivanov"}},
      {.node_id = scada::WellKnownRoleId(scada::WellKnownRole::kEngineer),
       .name = u"Engineer",
       .members = {}}};

  const auto by_account = RolesByAccount(roles);

  ASSERT_EQ(by_account.size(), 2u);
  const auto& held = by_account.at(u"ivanov");
  ASSERT_EQ(held.size(), 2u);
  EXPECT_EQ(held[0].node_id, op);
  EXPECT_EQ(held[0].name, u"Operator");
  EXPECT_EQ(held[1].node_id, admin);
  EXPECT_EQ(by_account.at(u"petrov").size(), 1u);
  // An account named by no rule is simply absent; the caller decides what that
  // means.
  EXPECT_FALSE(by_account.contains(u"audit"));
}

// A MethodService that records every call and answers each with the next
// status from `results` (Good once they run out) — the role editor's whole
// write path is these calls, so the test asserts on exactly what went out.
class RecordingMethodService : public scada::MethodService {
 public:
  struct Request {
    scada::NodeId node_id;
    scada::NodeId method_id;
    std::vector<scada::Variant> arguments;
  };

  scada::CoStatusOr<scada::CallResult> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::ServiceContext context) override {
    calls.push_back({node_id, method_id, arguments});
    scada::Status status = scada::StatusCode::Good;
    if (!results.empty()) {
      status = results.front();
      results.erase(results.begin());
    }
    co_return scada::MakeCallResult(std::move(status));
  }

  std::vector<Request> calls;
  std::vector<scada::Status> results;
};

// The editor computes the smallest set of changes: a Role the account already
// holds, or already lacks, is left alone, so an unchanged dialog writes
// nothing — and writes no audit record claiming a change.
TEST(RoleMembershipChangesTest, GrantsAndRevokesOnlyWhatDiffers) {
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  const scada::NodeId admin =
      scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);
  const scada::NodeId engineer =
      scada::WellKnownRoleId(scada::WellKnownRole::kEngineer);
  const std::vector<RoleMembership> roles = {
      {.node_id = op, .members = {u"ivanov"}},
      {.node_id = admin, .members = {u"petrov"}},
      {.node_id = engineer, .members = {u"ivanov"}}};

  const std::vector<scada::NodeId> desired = {op, admin};
  EXPECT_EQ(ComputeRoleMembershipChanges(roles, u"ivanov", desired),
            (std::vector<RoleMembershipChange>{
                {.role_id = admin, .grant = true},
                {.role_id = engineer, .grant = false}}));

  const std::vector<scada::NodeId> same = {op, engineer};
  EXPECT_TRUE(ComputeRoleMembershipChanges(roles, u"ivanov", same).empty());
}

// Anonymous and AuthenticatedUser are granted by token class; the server
// refuses AddIdentity on them (Part 18 §4.3), so the editor never asks.
TEST(RoleMembershipChangesTest, NeverTouchesTheTokenClassRoles) {
  const scada::NodeId anonymous =
      scada::WellKnownRoleId(scada::WellKnownRole::kAnonymous);
  const scada::NodeId authenticated =
      scada::WellKnownRoleId(scada::WellKnownRole::kAuthenticatedUser);
  EXPECT_FALSE(IsRoleMembershipEditable(anonymous));
  EXPECT_FALSE(IsRoleMembershipEditable(authenticated));
  EXPECT_TRUE(IsRoleMembershipEditable(
      scada::WellKnownRoleId(scada::WellKnownRole::kObserver)));
  EXPECT_TRUE(IsRoleMembershipEditable(scada::NodeId{5, 30}));

  const std::vector<RoleMembership> roles = {
      {.node_id = anonymous, .members = {}},
      {.node_id = authenticated, .members = {u"ivanov"}}};
  const std::vector<scada::NodeId> desired = {anonymous};
  EXPECT_TRUE(ComputeRoleMembershipChanges(roles, u"ivanov", desired).empty());
}

// What goes on the wire: one AddIdentity/RemoveIdentity per change, on the
// Role node, carrying a UserName rule naming the account — the shape the
// server's RoleSet manager decodes and the read path reads back.
TEST(RoleMembershipChangesTest, AppliesEachChangeAsAnIdentityCall) {
  RecordingMethodService methods;
  FakeNodeService node_service{scada::services{.method_service = &methods}};
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  const scada::NodeId admin =
      scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);
  node_service.Add(
      scada::NodeState{.node_id = op, .node_class = scada::NodeClass::Object});
  node_service.Add(scada::NodeState{.node_id = admin,
                                    .node_class = scada::NodeClass::Object});

  boost::asio::io_context io;
  std::optional<scada::Status> status;
  RunAwaitable(io, [&]() -> Awaitable<void> {
    status.emplace(co_await ApplyRoleMembershipChanges(
        node_service, u"Клиент 1",
        {{.role_id = op, .grant = true}, {.role_id = admin, .grant = false}}));
  });

  ASSERT_TRUE(status.has_value());
  EXPECT_TRUE(*status);
  ASSERT_EQ(methods.calls.size(), 2u);
  EXPECT_EQ(methods.calls[0].node_id, op);
  EXPECT_EQ(methods.calls[0].method_id,
            scada::NodeId{scada::security::id::RoleType_AddIdentity});
  EXPECT_EQ(methods.calls[1].node_id, admin);
  EXPECT_EQ(methods.calls[1].method_id,
            scada::NodeId{scada::security::id::RoleType_RemoveIdentity});
  for (const auto& call : methods.calls) {
    ASSERT_EQ(call.arguments.size(), 1u);
    const auto rule = scada::DecodeIdentityMappingRule(
        call.arguments[0].get_or(scada::ExtensionObject{}));
    ASSERT_TRUE(rule.has_value());
    EXPECT_EQ(rule->criteria_type, scada::IdentityCriteriaType::kUserName);
    EXPECT_EQ(rule->criteria, "Клиент 1");
  }
}

// A refused change stops the run and is reported: the operator must learn
// that the account does not hold what the dialog showed.
TEST(RoleMembershipChangesTest, StopsAtTheFirstRefusal) {
  RecordingMethodService methods;
  methods.results = {scada::StatusCode::Bad_UserAccessDenied};
  FakeNodeService node_service{scada::services{.method_service = &methods}};
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  node_service.Add(
      scada::NodeState{.node_id = op, .node_class = scada::NodeClass::Object});

  boost::asio::io_context io;
  std::optional<scada::Status> status;
  RunAwaitable(io, [&]() -> Awaitable<void> {
    status.emplace(co_await ApplyRoleMembershipChanges(
        node_service, u"ivanov",
        {{.role_id = op, .grant = true}, {.role_id = op, .grant = false}}));
  });

  ASSERT_TRUE(status.has_value());
  EXPECT_EQ(status->code(), scada::StatusCode::Bad_UserAccessDenied);
  EXPECT_EQ(methods.calls.size(), 1u);
}

}  // namespace
