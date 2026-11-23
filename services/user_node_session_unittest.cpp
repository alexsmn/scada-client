#include "services/user_node_session.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "model/security_node_ids.h"
#include "scada/session_service_mock.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

constexpr scada::NumericId kOperatorUserId = 12;

scada::ReferenceDescription UserReference(scada::NumericId id,
                                          const char* browse_name,
                                          std::u16string display_name) {
  return scada::ReferenceDescription{
      .reference_type_id = scada::id::Organizes,
      .node_id = scada::NodeId{id, scada::NamespaceIndexes::SCADA},
      .node_class = scada::NodeClass::Object,
      .browse_name = scada::QualifiedName{browse_name},
      .display_name =
          scada::LocalizedText{scada::String{"en"}, std::move(display_name)}};
}

class UserNodeSessionTest : public ::testing::Test {
 protected:
  UserNodeSessionTest() {
    ON_CALL(*inner_, GetUserId()).WillByDefault(Return(scada::NodeId{}));
    ON_CALL(*inner_, IsConnected(_)).WillByDefault(Return(true));
  }

  // The Users folder answers these references, and records what it was asked.
  void PublishUsers(std::vector<scada::ReferenceDescription> references) {
    EXPECT_CALL(*view_, Browse(_, _))
        .WillRepeatedly(
            [this, references](scada::ServiceContext,
                               std::vector<scada::BrowseDescription> inputs) {
              browsed_.insert(browsed_.end(), inputs.begin(), inputs.end());
              return scada::MakeViewResult<scada::BrowseResult>(
                  std::vector<scada::BrowseResult>{
                      scada::BrowseResult{.references = references}});
            });
  }

  scada::Status SignIn(std::u16string user_name) {
    return WaitAwaitable(executor_,
                         session_.ConnectStatus(scada::SessionConnectParams{
                             .user_name = scada::LocalizedText{user_name}}));
  }

  TestExecutor executor_;
  std::shared_ptr<NiceMock<scada::MockSessionService>> inner_ =
      std::make_shared<NiceMock<scada::MockSessionService>>();
  std::shared_ptr<NiceMock<scada::MockViewService>> view_ =
      std::make_shared<NiceMock<scada::MockViewService>>();
  UserNodeSessionService session_{inner_, view_};
  std::vector<scada::BrowseDescription> browsed_;
};

// Backlog 1110: the opcuapp session answers null, which the Client read as
// "no account" and so kept the profile off the Server.
TEST_F(UserNodeSessionTest, NamesTheAccountFoundInTheUsersFolder) {
  PublishUsers({UserReference(11, "admin", u"admin"),
                UserReference(kOperatorUserId, "operator", u"operator")});

  ASSERT_TRUE(scada::IsGood(SignIn(u"operator").code()));

  EXPECT_EQ(session_.GetUserId(),
            (scada::NodeId{kOperatorUserId, scada::NamespaceIndexes::SCADA}));
  ASSERT_EQ(browsed_.size(), 1u);
  EXPECT_EQ(browsed_[0].node_id, scada::NodeId{scada::security::id::Users});
  EXPECT_EQ(browsed_[0].direction, scada::BrowseDirection::Forward);
  EXPECT_EQ(browsed_[0].reference_type_id,
            scada::NodeId{scada::id::HierarchicalReferences});
}

// The Server resolves a login by DisplayName, whose locale the typed name
// does not carry.
TEST_F(UserNodeSessionTest, MatchesTheDisplayNameText) {
  PublishUsers({UserReference(kOperatorUserId, "user12", u"Диспетчер")});

  SignIn(u"Диспетчер");

  EXPECT_EQ(session_.GetUserId(),
            (scada::NodeId{kOperatorUserId, scada::NamespaceIndexes::SCADA}));
}

TEST_F(UserNodeSessionTest, AnUnknownNameStaysNull) {
  PublishUsers({UserReference(kOperatorUserId, "operator", u"operator")});

  SignIn(u"someone");

  EXPECT_TRUE(session_.GetUserId().is_null());
}

// root has no node in the folder; the Server answers it with RootUser.
TEST_F(UserNodeSessionTest, RootIsTheStaticRootUser) {
  PublishUsers({UserReference(kOperatorUserId, "operator", u"operator")});

  SignIn(u"root");

  EXPECT_EQ(session_.GetUserId(), scada::NodeId{scada::security::id::RootUser});
}

// A foreign server has no Users folder, and must not get a SCADA account.
TEST_F(UserNodeSessionTest, AnUnbrowsableFolderLeavesRootNull) {
  EXPECT_CALL(*view_, Browse(_, _)).WillRepeatedly([] {
    return scada::MakeViewResult<scada::BrowseResult>(
        std::vector<scada::BrowseResult>{scada::BrowseResult{
            .status_code = scada::StatusCode::Bad_WrongNodeId}});
  });

  SignIn(u"root");

  EXPECT_TRUE(session_.GetUserId().is_null());
}

TEST_F(UserNodeSessionTest, AnonymousDoesNotBrowse) {
  EXPECT_CALL(*view_, Browse(_, _)).Times(0);

  SignIn(u"");

  EXPECT_TRUE(session_.GetUserId().is_null());
}

TEST_F(UserNodeSessionTest, AFailedConnectDoesNotBrowse) {
  EXPECT_CALL(*view_, Browse(_, _)).Times(0);
  EXPECT_CALL(*inner_, ConnectStatus(_))
      .WillOnce([](scada::SessionConnectParams) -> scada::CoStatus {
        co_return scada::StatusCode::Bad_WrongLoginCredentials;
      });

  EXPECT_EQ(SignIn(u"operator").code(),
            scada::StatusCode::Bad_WrongLoginCredentials);
  EXPECT_TRUE(session_.GetUserId().is_null());
}

// A session that names its user itself is never second-guessed.
TEST_F(UserNodeSessionTest, KeepsTheInnerSessionsOwnUserId) {
  const scada::NodeId own{77, scada::NamespaceIndexes::SCADA};
  ON_CALL(*inner_, GetUserId()).WillByDefault(Return(own));
  EXPECT_CALL(*view_, Browse(_, _)).Times(0);

  SignIn(u"operator");

  EXPECT_EQ(session_.GetUserId(), own);
}

TEST_F(UserNodeSessionTest, DisconnectForgetsTheAccount) {
  PublishUsers({UserReference(kOperatorUserId, "operator", u"operator")});
  SignIn(u"operator");
  ASSERT_FALSE(session_.GetUserId().is_null());

  WaitAwaitable(executor_, session_.Disconnect());

  EXPECT_TRUE(session_.GetUserId().is_null());
}

}  // namespace
