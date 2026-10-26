#include "bulk_create/qt/bulk_create_wizard.h"

#include "aui/test/app_environment.h"
#include "bulk_create/qt/bulk_create_preview_panel.h"
#include "model/devices_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/node_id.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QComboBox>
#include <QLabel>
#include <QWizard>

#include <memory>
#include <vector>

namespace {

using testing::_;
using testing::NiceMock;

// The insert's answer; the wizard posts and does not wait for it.
scada::CoStatusOr<scada::NodeId> AcceptInsert() {
  co_return scada::NodeId{};
}

// The wizard is assembly over two pieces with their own tests -- the pattern
// engine and `PlanBulkCreate` -- so these assert the assembly: which subjects
// this invocation offers, and that Finish posts what the planner planned.

class BulkCreateWizardTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
  FakeNodeService node_service_;
  NiceMock<MockTaskManager> task_manager_;

  // A folder to create data items in, as the "Multiple create" entry passes.
  const scada::NodeId kFolder = scada::NodeId::FromString("ns=2;s=Folder");
  const scada::NodeId kReceiver = scada::NodeId::FromString("ns=3;s=DC");
  const scada::NodeId kRuleType = scada::NodeId::FromString("ns=3;s=RuleType");

  std::unique_ptr<QWizard> MakeWizard(
      std::vector<scada::NodeId> sources = {},
      std::vector<BulkCreateDestination> destinations = {}) {
    return std::unique_ptr<QWizard>{MakeBulkCreateWizard(
        {node_service_, task_manager_, kFolder, std::move(sources),
         std::move(destinations)})};
  }

  std::vector<BulkCreateDestination> Receivers() const {
    return {{.device_id = kReceiver, .name = u"DC", .rule_type_id = kRuleType}};
  }

  static QComboBox* SubjectCombo(QWizard& wizard) {
    return wizard.findChild<QComboBox*>(QStringLiteral("bulkCreateSubject"));
  }
};

// A transmission rule forwards an existing node. With no sources there is
// nothing to forward, so offering the subject would offer a run that creates
// nothing -- the caller decides which subjects exist by what it hands over.
TEST_F(BulkCreateWizardTest, WithoutSourcesOnlyTheDataItemSubjectIsOffered) {
  std::unique_ptr<QWizard> wizard = MakeWizard();
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  EXPECT_EQ(subject->count(), 1);
}

TEST_F(BulkCreateWizardTest, WithSourcesBothSubjectsAreOffered) {
  std::unique_ptr<QWizard> wizard =
      MakeWizard({scada::NodeId::FromString("ns=2;s=A")}, Receivers());
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  EXPECT_EQ(subject->count(), 2);
}

// The data item's binding fields have no meaning for a rule, so they go with
// the subject. Asserted on the *label* as well, because a QFormLayout label
// outlives the field it captions and would be left naming the row below it.
TEST_F(BulkCreateWizardTest, TheDataItemFieldsFollowTheSubject) {
  std::unique_ptr<QWizard> wizard =
      MakeWizard({scada::NodeId::FromString("ns=2;s=A")}, Receivers());
  wizard->show();
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  auto* device =
      wizard->findChild<QComboBox*>(QStringLiteral("bulkCreateDevice"));
  ASSERT_NE(device, nullptr);

  auto* destination =
      wizard->findChild<QComboBox*>(QStringLiteral("bulkCreateDestination"));
  ASSERT_NE(destination, nullptr);

  subject->setCurrentIndex(0);  // data items
  EXPECT_FALSE(device->isHidden());
  EXPECT_TRUE(destination->isHidden());

  subject->setCurrentIndex(1);  // transmission rules
  EXPECT_TRUE(device->isHidden());
  EXPECT_FALSE(destination->isHidden());
}

// Sources with nowhere to send them offer no rules: the subject would produce
// rules parented on nothing.
TEST_F(BulkCreateWizardTest, WithoutAReceivingDeviceTheRuleSubjectIsNotOffered) {
  std::unique_ptr<QWizard> wizard =
      MakeWizard({scada::NodeId::FromString("ns=2;s=A")});
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  EXPECT_EQ(subject->count(), 1);
}

// Backlog 894: the rules subject was unreachable, because the only entry
// passed no sources. Opened on a selection (no folder), the wizard is rules
// only, one per source, each parented on the chosen receiving device and typed
// by it -- the planner used to get the folder and no type at all.
TEST_F(BulkCreateWizardTest, OpenedOnSourcesCreatesOneRulePerSourceOnTheDevice) {
  const std::vector<scada::NodeId> sources = {
      scada::NodeId::FromString("ns=2;s=A"),
      scada::NodeId::FromString("ns=2;s=B")};
  auto wizard = std::unique_ptr<QWizard>{MakeBulkCreateWizard(
      {.node_service_ = node_service_,
       .task_manager_ = task_manager_,
       .source_node_ids_ = sources,
       .destinations_ = Receivers()})};
  QComboBox* subject = SubjectCombo(*wizard);
  ASSERT_NE(subject, nullptr);
  ASSERT_EQ(subject->count(), 1);

  std::vector<scada::NodeState> posted;
  EXPECT_CALL(task_manager_, PostInsertTask(_))
      .Times(2)
      .WillRepeatedly([&posted](const scada::NodeState& node) {
        posted.push_back(node);
        return AcceptInsert();
      });
  wizard->accept();

  ASSERT_EQ(posted.size(), 2u);
  for (std::size_t i = 0; i < posted.size(); ++i) {
    EXPECT_EQ(posted[i].parent_id, kReceiver);
    EXPECT_EQ(posted[i].type_definition_id, kRuleType);
    EXPECT_TRUE(posted[i].node_id.is_null()) << "the server allocates rule ids";
    EXPECT_EQ(scada::GetProperty(
                  posted[i].properties,
                  scada::devices::id::TransmissionItemType_SourceNode),
              scada::Variant{sources[i]});
  }
}

// Finish posts one insert per planned node, and the Review page's count is the
// same number -- both come from PlanBulkCreate, which is the point of having
// one planner.
TEST_F(BulkCreateWizardTest, FinishPostsOneInsertPerPlannedNode) {
  std::unique_ptr<QWizard> wizard = MakeWizard();

  auto* preview = wizard->findChild<BulkCreatePreviewPanel*>();
  ASSERT_NE(preview, nullptr);
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = u"AI{n}";
  params.node_id_template = u"ns=2;s=AI{n}";
  params.count = 4;
  preview->SetParams(params);

  EXPECT_CALL(task_manager_, PostInsertTask(_)).Times(4);
  wizard->accept();
}

// Nothing to create must post nothing rather than an empty run.
TEST_F(BulkCreateWizardTest, FinishWithNoRowsPostsNothing) {
  std::unique_ptr<QWizard> wizard = MakeWizard();

  auto* preview = wizard->findChild<BulkCreatePreviewPanel*>();
  ASSERT_NE(preview, nullptr);
  BulkCreateParams params;
  params.count = 0;
  preview->SetParams(params);

  EXPECT_CALL(task_manager_, PostInsertTask(_)).Times(0);
  wizard->accept();
}

}  // namespace
