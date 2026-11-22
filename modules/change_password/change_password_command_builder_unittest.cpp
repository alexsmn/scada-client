#include "modules/change_password/change_password_command_builder.h"

#include "aui/dialog_service_mock.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "core/global_command_context.h"
#include "events/local_events.h"
#include "main_window/main_window_mock.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/test/fake_node_service.h"
#include "profile/profile.h"
#include "resources/common_resources.h"
#include "scada/access_rights.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/session_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

#include <optional>

using namespace testing;

namespace {

constexpr scada::NumericId kOwnUserId = 5001;

const scada::NodeId kOwnUser{kOwnUserId, 1};
const scada::NodeId kUserManagement{
    scada::id::Server_ServerConfiguration_UserManagement,
    scada::NamespaceIndexes::NS0};
const scada::NodeId kChangePasswordMethod{
    scada::id::UserManagement_ChangePassword, scada::NamespaceIndexes::NS0};

// The fields of a ChangePasswordContext the test reads back. The context
// itself holds references and a const member, so it cannot be copied out of
// the dialog callback; the request is sent from inside it instead.
struct OpenedForm {
  scada::NodeId user;
  bool self_service = false;
};

class ChangeOwnPasswordCommandTest : public Test {
 protected:
  ChangeOwnPasswordCommandTest() {
    node_service_.Add(scada::NodeState{
        .node_id = kOwnUser,
        .attributes = {.display_name = scada::LocalizedText{u"dispatcher"}}});
    node_service_.Add(scada::NodeState{.node_id = kUserManagement});

    // A dispatcher: signed in, named, and holding no right beyond Control --
    // in particular not Configure, which "Set Password..." requires.
    ON_CALL(session_service_, IsConnected(_)).WillByDefault(Return(true));
    ON_CALL(session_service_, GetUserId()).WillByDefault(Return(kOwnUser));
    ON_CALL(session_service_, IsAnonymous()).WillByDefault(Return(false));
    ON_CALL(session_service_, GetAccessRights())
        .WillByDefault(Return(static_cast<std::uint32_t>(
            1u << static_cast<unsigned>(scada::AccessRight::kControl))));
  }

  // Builds the command with a dialog stand-in that records what the form was
  // opened for and, when `passwords` is given, submits it with them.
  BasicCommand<GlobalCommandContext> Build(
      std::optional<std::pair<std::u16string, std::u16string>> passwords =
          std::nullopt) {
    return ChangeOwnPasswordCommandBuilder{
        .executor_ = executor_,
        .local_events_ = local_events_,
        .profile_ = profile_,
        .session_service_ = session_service_,
        .node_service_ = node_service_,
        .show_dialog_ =
            [this, passwords](DialogService&, ChangePasswordContext&& context) {
              opened_ = OpenedForm{.user = context.user_.node_id(),
                                   .self_service = context.self_service_};
              if (passwords) {
                ChangePassword(context, passwords->first, passwords->second);
              }
            }}
        .Build();
  }

  GlobalCommandContext CommandContext() {
    return GlobalCommandContext{.main_window = main_window_,
                                .dialog_service = dialog_service_};
  }

  TestExecutor executor_;
  StrictMock<scada::MockMethodService> method_service_;
  FakeNodeService node_service_{
      scada::services{.method_service = &method_service_}};
  NiceMock<scada::MockSessionService> session_service_;
  LocalEvents local_events_;
  Profile profile_;
  NiceMock<MockMainWindow> main_window_;
  NiceMock<MockDialogService> dialog_service_;
  std::optional<OpenedForm> opened_;
};

// Backlog 1098: a dispatcher without the Configure right, with nothing
// selected, is offered a change of their own password.
TEST_F(ChangeOwnPasswordCommandTest, OfferedWithoutConfigureRight) {
  ASSERT_FALSE(session_service_.HasAccessRight(scada::AccessRight::kConfigure));

  BasicCommand<GlobalCommandContext> command = Build();

  EXPECT_EQ(command.command_id, ID_CHANGE_OWN_PASSWORD);
  ASSERT_TRUE(command.available_handler);
  EXPECT_TRUE(command.available_handler(CommandContext()));
  EXPECT_FALSE(command.enabled_handler);
}

// The form opens for the session's own account on the self-service path,
// whatever the selection: there is no selection in a global command.
TEST_F(ChangeOwnPasswordCommandTest, OpensTheFormForTheSessionsOwnAccount) {
  BasicCommand<GlobalCommandContext> command = Build();

  command.execute_handler(CommandContext());

  ASSERT_TRUE(opened_.has_value());
  EXPECT_EQ(opened_->user, kOwnUser);
  EXPECT_TRUE(opened_->self_service);
}

// An anonymous session has no account and so no password; a disconnected
// client has no session to ask.
TEST_F(ChangeOwnPasswordCommandTest, NotOfferedAnonymouslyOrDisconnected) {
  BasicCommand<GlobalCommandContext> command = Build();

  EXPECT_CALL(session_service_, IsAnonymous()).WillRepeatedly(Return(true));
  EXPECT_FALSE(command.available_handler(CommandContext()));

  EXPECT_CALL(session_service_, IsAnonymous()).WillRepeatedly(Return(false));
  EXPECT_CALL(session_service_, IsConnected(_)).WillRepeatedly(Return(false));
  EXPECT_FALSE(command.available_handler(CommandContext()));

  command.execute_handler(CommandContext());
  EXPECT_FALSE(opened_.has_value());
}

// Against a 2.6 Server the request is UserManagement.ChangePassword (OPC UA
// Part 18 §5.2.8) carrying the CURRENT password and the new one -- the call a
// non-administrator is allowed to make.
TEST_F(ChangeOwnPasswordCommandTest, SendsTheCurrentPasswordTo26Server) {
  std::vector<scada::Variant> sent;
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, _, _))
      .WillOnce([&sent](auto, auto, auto arguments, auto) {
        sent = arguments;
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      });

  Build(std::pair{std::u16string{u"current"}, std::u16string{u"new"}})
      .execute_handler(CommandContext());
  executor_.Poll();

  ASSERT_EQ(sent.size(), 2u);
  EXPECT_EQ(sent[0].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"current"});
  EXPECT_EQ(sent[1].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"new"});
  ASSERT_EQ(local_events_.events().size(), 1u);
  EXPECT_EQ(local_events_.events().front()->severity, scada::kSeverityNormal);
}

// A 2.5.7 Server has no UserManagement object and changes a password only
// together with the current one, through the per-user ChangePassword method on
// the account's own node. The self-service command reaches it there.
TEST_F(ChangeOwnPasswordCommandTest, SendsTheCurrentPasswordTo257Server) {
  std::vector<scada::Variant> sent;
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, _, _))
      .WillOnce([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Bad_WrongNodeId);
      });
  EXPECT_CALL(
      method_service_,
      Call(kOwnUser, scada::security::id::UserType_ChangePassword, _, _))
      .WillOnce([&sent](auto, auto, auto arguments, auto) {
        sent = arguments;
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      });

  Build(std::pair{std::u16string{u"current"}, std::u16string{u"new"}})
      .execute_handler(CommandContext());
  executor_.Poll();

  ASSERT_EQ(sent.size(), 2u);
  EXPECT_EQ(sent[0].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"current"});
  EXPECT_EQ(sent[1].get_or(scada::LocalizedText{}),
            scada::LocalizedText{u"new"});
  ASSERT_EQ(local_events_.events().size(), 1u);
  EXPECT_EQ(local_events_.events().front()->severity, scada::kSeverityNormal);
}

}  // namespace
