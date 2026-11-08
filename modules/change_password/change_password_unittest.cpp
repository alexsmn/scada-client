#include "modules/change_password/change_password.h"

#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "events/local_events.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/test/fake_node_service.h"
#include "profile/profile.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kUserNodeId = 5001;

// Both standard methods live on the UserManagement object, not on the per-user
// node: the standard user model keys accounts by NAME and has no node to call
// a method on (OPC UA Part 18 §5.2).
const scada::NodeId kUserManagement{
    scada::id::Server_ServerConfiguration_UserManagement,
    scada::NamespaceIndexes::NS0};
const scada::NodeId kChangePasswordMethod{
    scada::id::UserManagement_ChangePassword, scada::NamespaceIndexes::NS0};
const scada::NodeId kModifyUserMethod{scada::id::UserManagement_ModifyUser,
                                      scada::NamespaceIndexes::NS0};

}  // namespace

class ChangePasswordTest : public Test {
 protected:
  ChangePasswordTest()
      : user_node_{node_service_.Add(scada::NodeState{
            .node_id = scada::NodeId{kUserNodeId, 1},
            .attributes = {.display_name =
                               scada::LocalizedText{u"Operator"}}})} {
    node_service_.Add(scada::NodeState{.node_id = kUserManagement});
  }

  ChangePasswordContext Context(bool self_service) {
    return ChangePasswordContext{.user_ = user_node_,
                                 .executor_ = executor_,
                                 .local_events_ = local_events_,
                                 .profile_ = profile_,
                                 .node_service_ = &node_service_,
                                 .self_service_ = self_service};
  }

  void PollExecutor() { executor_.Poll(); }

  TestExecutor executor_;
  StrictMock<scada::MockMethodService> method_service_;
  // The fake backs GetScadaNode() with these services, so method calls made
  // through the node cursor land on |method_service_|. Declared after it so it
  // outlives nothing it depends on.
  FakeNodeService node_service_{
      scada::services{.method_service = &method_service_}};
  NodeRef user_node_;
  LocalEvents local_events_;
  Profile profile_;
};

// Changing one's OWN password is Part 18 §5.2.8: it proves the old password,
// so both arguments go across.
TEST_F(ChangePasswordTest, SelfServiceCallsTheStandardChangePassword) {
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, SizeIs(2), _))
      .WillOnce([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      });

  ChangePassword(Context(/*self_service=*/true), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityNormal);
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
            std::u16string::npos);
}

// An ADMINISTRATOR resetting someone else's account is §5.2.7 ModifyUser,
// which does not require the old password — so it must not be sent, and the
// account is named rather than pointed at by node. Seven arguments per the
// spec's signature.
TEST_F(ChangePasswordTest, AdminResetCallsModifyUserAndSendsNoOldPassword) {
  std::vector<scada::Variant> sent;
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kModifyUserMethod, SizeIs(7), _))
      .WillOnce([&sent](auto, auto, auto arguments, auto) {
        sent = arguments;
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      });

  ChangePassword(Context(/*self_service=*/false), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(sent.size(), 7u);
  // The account is identified by name, and the old password appears nowhere.
  EXPECT_EQ(sent[0].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"Operator"});
  EXPECT_TRUE(sent[1].get_or(false));
  EXPECT_EQ(sent[2].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"new"});
  for (const scada::Variant& argument : sent) {
    EXPECT_NE(argument.get_or(scada::LocalizedText{}),
              scada::LocalizedText{u"old"});
  }
}

// A 2.5.7 Server has no UserManagement object: it refuses the standard call
// as an unknown node and changes a password with the per-user ChangePassword
// method, which takes the current password and the new one. One's own
// password must still change there, the way the 2.5 Client changed it.
TEST_F(ChangePasswordTest, SelfServiceOnA257ServerUsesThePerUserMethod) {
  const scada::NodeId legacy_method =
      scada::security::id::UserType_ChangePassword;
  std::vector<scada::Variant> sent;
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, _, _))
      .WillOnce([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Bad_WrongNodeId);
      });
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kUserNodeId, 1}, legacy_method, _, _))
      .WillOnce([&sent](auto, auto, auto arguments, auto) {
        sent = arguments;
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      });

  ChangePassword(Context(/*self_service=*/true), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(sent.size(), 2u);
  EXPECT_EQ(sent[0].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"old"});
  EXPECT_EQ(sent[1].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"new"});
  ASSERT_EQ(local_events_.events().size(), 1);
  EXPECT_EQ(local_events_.events().front()->severity, scada::kSeverityNormal);
}

// An administrator's reset has no 2.5 equivalent -- the 2.5.7 Server always
// demands the account's current password -- so nothing is retried and the
// report says what to do instead of naming a status code.
TEST_F(ChangePasswordTest, AdminResetOnA257ServerExplainsInsteadOfFailing) {
  EXPECT_CALL(method_service_, Call(kUserManagement, kModifyUserMethod, _, _))
      .WillOnce([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Bad_WrongNodeId);
      });

  ChangePassword(Context(/*self_service=*/false), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.text.find(u"2.6"), std::u16string::npos);
}

TEST_F(ChangePasswordTest, ReportsFailureAfterMethodCallCompletes) {
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, SizeIs(2), _))
      .WillOnce([](auto, auto, auto, auto) {
        // A refusal that is not "no such method": Bad_WrongMethodId would
        // now be read as a pre-2.6 Server and retried the 2.5 way.
        return scada::MakeMethodCallResult(
            scada::StatusCode::Bad_UserAccessDenied);
      });

  ChangePassword(Context(/*self_service=*/true), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
            std::u16string::npos);
}

// The Server refuses a change to root's password with Bad_NotWritable when the
// server computer configures it, because every start would rewrite it there.
// The bare status name would not tell the operator where the password IS
// changed, so the report names the command.
// The server answers Bad_NotWritable for root when its password is configured
// on the server computer, and for the inter-process service account `svc`
// always; each refusal names the command that changes that password (backlog
// 882).
TEST_F(ChangePasswordTest, NotWritableReportsWhereThePasswordIsChanged) {
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kModifyUserMethod, SizeIs(7), _))
      .Times(2)
      .WillRepeatedly([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Bad_NotWritable);
      });

  const NodeRef root = node_service_.Add(scada::NodeState{
      .node_id = scada::NodeId{kUserNodeId + 1, 1},
      .attributes = {.display_name = scada::LocalizedText{u"root"}}});
  const NodeRef svc = node_service_.Add(scada::NodeState{
      .node_id = scada::NodeId{kUserNodeId + 2, 1},
      .attributes = {.display_name = scada::LocalizedText{u"svc"}}});

  for (const NodeRef& user : {root, svc}) {
    ChangePasswordContext context = Context(/*self_service=*/false);
    ChangePassword(ChangePasswordContext{.user_ = user,
                                         .executor_ = context.executor_,
                                         .local_events_ = context.local_events_,
                                         .profile_ = context.profile_,
                                         .node_service_ = context.node_service_,
                                         .self_service_ = false},
                   u"", u"new");
    PollExecutor();
  }

  ASSERT_EQ(local_events_.events().size(), 2);
  const auto& root_event = *local_events_.events()[0];
  EXPECT_EQ(root_event.severity, scada::kSeverityCritical);
  EXPECT_NE(root_event.message.text.find(u"Changing password for user root"),
            std::u16string::npos);
  EXPECT_NE(
      root_event.message.text.find(u"scada-setup set-password --root-password"),
      std::u16string::npos);
  const auto& svc_event = *local_events_.events()[1];
  EXPECT_NE(
      svc_event.message.text.find(u"scada-setup set-password --svc-password"),
      std::u16string::npos);
}
