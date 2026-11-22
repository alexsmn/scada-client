#include "modules/change_password/change_password_dialog.h"

#include "aui/dialog_service_mock.h"
#include "aui/test/app_environment.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "events/local_events.h"
#include "model/namespaces.h"
#include "node_service/test/fake_node_service.h"
#include "profile/profile.h"
#include "scada/authorization.h"
#include "scada/localized_text.h"
#include "scada/range_encoding.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <memory>

namespace {

const scada::NodeId kUser{5001, 1};
const scada::NodeId kUserManagement{
    scada::id::Server_ServerConfiguration_UserManagement,
    scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordLength{scada::id::UserManagement_PasswordLength,
                                    scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordOptions{scada::id::UserManagement_PasswordOptions,
                                     scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordRestrictions{
    scada::id::UserManagement_PasswordRestrictions,
    scada::NamespaceIndexes::NS0};

// Drives the real Qt form: what the operator sees once the server's policy
// arrives, and whether OK lets a password through that the server would
// reject (backlog 1100: nothing ever set the policy, so nothing was checked).
class ChangePasswordDialogTest : public testing::Test {
 protected:
  ChangePasswordDialogTest() {
    node_service_.Add(scada::NodeState{.node_id = kUser});
  }

  ~ChangePasswordDialogTest() override { delete FindDialog(); }

  // Publishes a policy: 8 to 16 characters, a digit, and the deployment's
  // own sentence.
  void PublishPolicy() {
    node_service_.Add(scada::NodeState{.node_id = kUserManagement});
    node_service_.Add(scada::NodeState{
        .node_id = kPasswordLength,
        .attributes = {.value = scada::Variant{
                           scada::MakeRangeObject({.low = 8, .high = 16})}}});
    node_service_.Add(scada::NodeState{
        .node_id = kPasswordOptions,
        .attributes = {
            .value = scada::Variant{static_cast<scada::UInt32>(
                scada::PasswordOptions::kRequiresDigitCharacters)}}});
    node_service_.Add(scada::NodeState{
        .node_id = kPasswordRestrictions,
        .attributes = {.value = scada::Variant{scada::LocalizedText{
                           u"Change it every 90 days."}}}});
  }

  void Open() {
    ShowChangePasswordDialog(
        dialog_service_,
        ChangePasswordContext{.user_ = node_service_.GetNode(kUser),
                              .executor_ = executor_,
                              .local_events_ = local_events_,
                              .profile_ = profile_,
                              .node_service_ = &node_service_,
                              .self_service_ = true});
  }

  static QDialog* FindDialog() {
    for (QWidget* widget : QApplication::topLevelWidgets()) {
      if (widget->objectName() == QStringLiteral("ChangePasswordDialog"))
        return qobject_cast<QDialog*>(widget);
    }
    return nullptr;
  }

  static QWidget* Rules(QDialog& dialog) {
    return dialog.findChild<QWidget*>(QStringLiteral("changePasswordRules"));
  }

  static QStringList RuleTexts(QDialog& dialog) {
    QStringList texts;
    for (const QLabel* label : Rules(dialog)->findChildren<QLabel*>())
      texts << label->text();
    return texts;
  }

  static QPushButton* Ok(QDialog& dialog) {
    return dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok);
  }

  static void TypeNew(QDialog& dialog, const QString& password) {
    dialog.findChild<QLineEdit*>(QStringLiteral("newLineEdit"))
        ->setText(password);
  }

  AppEnvironment app_env_;
  TestExecutor executor_;
  FakeNodeService node_service_;
  LocalEvents local_events_;
  Profile profile_;
  testing::NiceMock<MockDialogService> dialog_service_;
};

TEST_F(ChangePasswordDialogTest, ListsTheServersRulesAndGatesOk) {
  PublishPolicy();
  Open();
  QDialog* dialog = FindDialog();
  ASSERT_NE(dialog, nullptr);

  ASSERT_NE(Rules(*dialog), nullptr);
  // The read has not been answered yet: nothing is claimed, nothing blocked.
  EXPECT_FALSE(Rules(*dialog)->isVisibleTo(dialog));
  EXPECT_TRUE(Ok(*dialog)->isEnabled());

  executor_.Poll();

  ASSERT_TRUE(Rules(*dialog)->isVisibleTo(dialog));
  EXPECT_THAT(RuleTexts(*dialog),
              testing::UnorderedElementsAre(
                  QStringLiteral("· Length: 8–16"), QStringLiteral("· Digit"),
                  QStringLiteral("Change it every 90 days.")));
  EXPECT_FALSE(Ok(*dialog)->isEnabled());

  TypeNew(*dialog, QStringLiteral("abcdefgh"));  // long enough, no digit
  EXPECT_TRUE(RuleTexts(*dialog).contains(QStringLiteral("✓ Length: 8–16")));
  EXPECT_TRUE(RuleTexts(*dialog).contains(QStringLiteral("· Digit")));
  EXPECT_FALSE(Ok(*dialog)->isEnabled());

  TypeNew(*dialog, QStringLiteral("abcdefg1"));
  EXPECT_TRUE(RuleTexts(*dialog).contains(QStringLiteral("✓ Digit")));
  EXPECT_TRUE(Ok(*dialog)->isEnabled());
}

// An unreadable policy is not "no rules": the block stays absent and the
// server alone judges, so a password the client cannot check still goes out.
TEST_F(ChangePasswordDialogTest, UnreadablePolicyImposesNothing) {
  Open();
  QDialog* dialog = FindDialog();
  ASSERT_NE(dialog, nullptr);
  ASSERT_NE(Rules(*dialog), nullptr);

  executor_.Poll();
  TypeNew(*dialog, QStringLiteral("x"));

  EXPECT_FALSE(Rules(*dialog)->isVisibleTo(dialog));
  EXPECT_TRUE(Ok(*dialog)->isEnabled());
}

// Closing the form before the server answers drops the answer.
TEST_F(ChangePasswordDialogTest, ClosedBeforeTheAnswerIsSafe) {
  PublishPolicy();
  Open();
  delete FindDialog();

  executor_.Poll();

  EXPECT_EQ(FindDialog(), nullptr);
}

}  // namespace
